#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/types.h>

#include "i8254.h"
#include "timer.h"
#include "keyboard.h"
#include "i8042.h"

static int hook_id; //Timer 0 hook id
static uint32_t counter = 0; //Interrupt handler counter, global to avoid changing a function that already came defined - void timer_int_handler()

//TODO TEMP
static uint16_t* notes = NULL;
static uint16_t* duration = NULL;
static uint16_t loop = 0;

//Clears Minix terminal screen by using ANSI escape codes - to use compile using "make CLRSCR=1"
void clearscreen() {
#if defined(CLRSCR) && CLRSCR == 1
	printf( "\033[2J" ); //ANSI escape clear code
#endif
}


//Configure timer initial counter value
int timer_set_square(unsigned long timer, unsigned long freq) {

	//Read status byte from user supplied timer so we can preserve the first 4 bits of configuration
	unsigned long status;
	if (sys_outb(TIMER_CTRL, TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer)) != OK) {
		printf("timer: timer_set_square: failed write to timer control register\n");
		return -1;
	}
	if (sys_inb(TIMER(timer), &status) != OK) { //TIMER_0 + timer results in TIMER_0, TIMER_1 and TIMER_2 eliminating the need for conditionals
		printf("timer: timer_set_square: failed to read timer status\n");
		return -1;
	}

	//Print timer status byte for debugging
#if defined(DEBUG) && DEBUG == 1
	printf("Status byte of timer %d: 0x%02X\n\n", timer, status);
#endif

	//Write timer configuration to control register
	if (sys_outb(TIMER_CTRL, TIMER_SEL(timer) | TIMER_LSB_MSB | (status & 0x0F)) != OK) { //(status & 0x0F) preserves the first 4 bits of configuration
		printf("timer: timer_set_square: failed write to timer control register\n");
		return -1;
	}

	//Calculate counter value to initialize timer register with
	int div = TIMER_FREQ / freq; //freq - counter value increment per second

	//Validate div value
	//"The fastest possible interrupt frequency is a little over a half of a megahertz
	//The slowest possible frequency, which is also the one normally used by computers running MS-DOS or compatible operating systems, is about 18.2 Hz"
	//-- Source: Wikipedia on Intel 8254
	if (freq <= COUNTER_MIN) { //18.2Hz gives a div value of 65535, the maximum a 16bit unsigned value can take
		printf("Frequency should be between 18.2Hz and 596591Hz\n");
		printf("Set timer %d frequency to 18.2Hz\n\n", timer);
		div = MAX_UINT16; //Force 18.2Hz
	} else if (freq > COUNTER_MAX) { //596591Hz gives a div value of 2, div = 1 is not allowed in Mode 3 and div = 0 should be implemented as 65536
		printf("Frequency should be between 18.2Hz and 596591Hz\n");
		printf("Set timer %d frequency to 596591Hz\n\n", timer);
		div = 2; //Force 596591Hz
	} else {
		int realfreq = TIMER_FREQ / div; //Check if counter is capable of representing requested frequency
		if (realfreq != freq) {
			printf("%dHz is not allowed, rounding up to nearest allowed frequency: %dHz\n\n", freq, realfreq);
		} else printf("Set timer %d frequency to %d\n\n", timer, freq);
	}

	//Print div for debugging
#if defined(DEBUG) && DEBUG == 1
	printf("Counter initialized to: %d\n\n", div & 0xFFFF);
#endif

	//Write counter value to user supplied timer register
	if (sys_outb(TIMER(timer), div & 0xFF) != OK) { //Write LSB of div
		printf("timer: timer_set_square: failed to initialize counter value\n");
		return -1;
	}
	if (sys_outb(TIMER(timer), (div >> 8) & 0xFF) != OK) { //Write MSB of div
		printf("timer: timer_set_square: failed to initialize counter value\n");
		return -1;
	}

	return 0;
}


//Subscribe timer interrupts
int timer_subscribe_int(void ) {
	hook_id = TIMER_HOOK_BIT;
    if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id) == OK) {
    	if (sys_irqenable(&hook_id) == OK) {
#if defined(DEBUG) && DEBUG == 1
    		printf("Hook id returned: 0x%02X\n\n", hook_id);
#endif
    		return TIMER_HOOK_BIT;
    	} else {
    		printf("timer: timer_subcribe_int failed to enable interrupts\n");
    		return -1;
    	}
    } else {
    	printf("timer: timer_subscribe_int failed to set policy\n");
    	return -1;
    }
}


//Unsubscribe timer interrupts
int timer_unsubscribe_int() {
    if(sys_irqdisable(&hook_id) == OK) {
    	if(sys_irqrmpolicy(&hook_id) == OK) {
    		return 0;
    	} else {
    		printf("timer: timer_unsubscribe_int failed to disable policy\n");
    		return -1;
    	}
    } else {
    		printf("timer: timer_unsubscribe_int failed to disable interrupts\n");
    		return -1;
    }
}


//Increments counter for the timer interrupt handler
void timer_int_handler() {
	counter++;
}


//Read status byte of timer using read-back command
int timer_get_conf(unsigned long timer, unsigned char *st) {

	//Check if timer supplied by user is within range [0-2]
	if (timer < 0 || timer > 2) {
		printf("timer: timer_get_conf: timer must be an integer ranging from 0 to 2\n");
		return ERROR;
	}

//Print timer counter value for debugging
#if defined(DEBUG) && DEBUG == 1
	unsigned long lsb;
	unsigned long msb;
	if (sys_outb(TIMER_CTRL, TIMER_SEL(timer)) != OK) //TIMER_SEL(timer) results in TIMER_SEL0, TIMER_SEL1 and TIMER_SEL2 eliminating the need for conditionals
		return ERROR;
	if (sys_inb(TIMER(timer), &lsb) != OK)
		return ERROR;
	if (sys_inb(TIMER(timer), &msb) != OK)
		return ERROR;
	printf("Counter value: %d\n", lsb | (msb << 8));
#endif

	//Write read-back command with user supplied timer to timer control register
	unsigned long status;
	if (sys_outb(TIMER_CTRL, TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer)) != OK) {
		printf("timer: timer_get_conf: failed write to timer control register\n");
		return ERROR;
	}

	//Read status byte from user supplied timer
	if (sys_inb(TIMER(timer), &status) != OK) { //TIMER_0 + timer results in TIMER_0, TIMER_1 and TIMER_2 eliminating the need for conditionals
		printf("timer: timer_get_conf: failed to read timer status\n");
		return ERROR;
	}

//Print timer status byte for debugging
#if defined(DEBUG) && DEBUG == 1
	printf("Status byte of timer %d: 0x%02X\n\n", timer, status);
#endif

	return status;
}


//Display status byte of timer in a human friendly way
int timer_display_conf(unsigned char conf) {

	//Counting mode bit parse
	printf("Counting mode:   ");
	if ((conf & TIMER_BCD) == TIMER_BCD) {
		printf("BCD\n");
	} else {
		printf("Binary\n");
	}

	//Programmed mode bits parse
	printf("Programmed mode: ");
	if ((conf & TIMER_SQR_WAVE) == TIMER_SQR_WAVE)
		printf("Mode 3 (Square Wave Mode)\n");
	else if ((conf & TIMER_MODE_5) == TIMER_MODE_5)
		printf("Mode 5 (Hardware Triggered Strobe)\n");
	else if ((conf & TIMER_MODE_1) == TIMER_MODE_1)
		printf("Mode 1 (Hardware Retriggerable One-Shot)\n");
	else if ((conf & TIMER_MODE_4) == TIMER_MODE_4)
		printf("Mode 4 (Software Triggered Strobe)\n");
	else if ((conf & TIMER_RATE_GEN) == TIMER_RATE_GEN)
		printf("Mode 2 (Rate Generator)\n");
	else printf("Mode 0 (Interrupt On Terminal Count)\n");

	//Type of access bits parse
	printf("Type of access:  ");
	if ((conf & TIMER_LSB_MSB) == TIMER_LSB_MSB)
		printf("LSB followed by MSB\n");
	else if ((conf & TIMER_LSB) == TIMER_LSB)
		printf("LSB\n");
	else if ((conf & TIMER_MSB) == TIMER_MSB)
		printf("MSB\n");
	else printf("Counter Latch Command\n");

	//Output line bit parse
	printf("Output:          ");
	if ((conf & TIMER_OUT) == TIMER_OUT)
		printf("High\n");
	else printf("Low\n");

	printf("\n"); //Padding
	return 0;
}


//Function for testing setting timer 0 to a different frequency
int timer_test_square(unsigned long freq) {

	//Set timer 0 to user supplied frequency
	clearscreen();
	if (timer_set_square(0, freq) == 0) {
		return 0;
	}

	return -1; //Most likely MINIX failed on any of the sys calls
}


//Tests timer interrupts by counting up to time (in seconds), works with any valid frequency
int timer_test_int(unsigned long time) {

	//Limit time to reasonable values
	if (time < 0 || time > 300) {
		printf("timer: timer_test_int: time value is restricted to a range from 0 to 300 seconds\n");
		return -1;
	}

	//Subscribe timer interrupts
	clearscreen();
	int hook = timer_subscribe_int();
	if (hook >= 0) { //Check if valid hook id

		//Variables for interrupt handling
		int ipc_status, dstatus;
		message msg;
		int irq_set = BIT(hook); //bitmask for timer interrupts
		int freq = 60; //Frequency used by the timer interrupt

		 //Since function only has time parameter, assuming 60Hz as frequency but code allows any frequency
//		if (timer_set_square(0, freq) != 0) {
//			timer_unsubscribe_int(); //No need to check since an error is already being returned
//			return -1; //Failed to set timer 0 to freq
//		}

		//Interrupt handling
		while(counter < time) { //counter is updated at freq/s so we multiply freq * time to get counter value at time (seconds)
			if ((dstatus = driver_receive(ANY, &msg, &ipc_status)) != 0) { //Receives any messages
				printf("driver_receive failed with: %d", dstatus);
				continue;
			}

			//Check if notification
			if (is_ipc_notify(ipc_status)) {
			//Debug info for notifications and interrupt bitmask
#if defined(DEBUG) && DEBUG == 1
	printf("Notification received\n");
	printf("Interrupt bitmask: 0x%02X\n\n", msg.NOTIFY_ARG);
#endif
				switch (_ENDPOINT_P(msg.m_source)) {
					case HARDWARE: //Hardware interrupt notification
						if (msg.NOTIFY_ARG & irq_set) { //Bitmask hook id to check for timer interrupt
							counter++;
						}
						break;

					default: //No other notifications expected, do nothing
						break;
				}
			} else {
				//Debug info for standard message reception
#if defined(DEBUG) && DEBUG == 1
	printf("Standard message received\n\n");
#endif
				}
		}
		printf("\n"); //Padding

		//Reset parameters
		counter = 0;
//		if (freq != 60) {
//			if (timer_set_square(0, 60) != 0) {
//				timer_unsubscribe_int(); //No need to check since an error is already being returned
//				return -1; //Failed to set timer 0 to 60Hz
//			}
//		}

		//Unsubscribe timer interrupts
		if (timer_unsubscribe_int() == 0) {
			return 0;
		} else return -1; //Failed to unsubscribe timer interrupts
	} else return -1; //Failed to subscribe timer interrupts
}


//Function for testing reading timer config and displaying it
int timer_test_config(unsigned long timer) {

	//Read config from user supplied timer
	unsigned char status;
	unsigned long st;
	clearscreen();
	st = timer_get_conf(timer, &status);

	//Display config in human friendly way
	if (st != ERROR) {
		unsigned char status = st;
		printf("Configuration of timer %d\n", timer);
		printf("--------------------------\n\n");
		timer_display_conf(status);
		return 0;
	}

	return -1; //Most likely MINIX failed on any of the sys calls
}


void call_timer2(uint8_t sample) {
	unsigned long test = sample & 0xFF;
	sys_outb(TIMER_2, test);
}


uint8_t round(double d)
{
    return (d + 0.5);
};


int8_t disable_speaker() {
	unsigned long status;
	sys_inb(SPEAKER_CTRL, &status);
	sys_outb(SPEAKER_CTRL, status & 0xFC);
}


int8_t timer_test_speaker(uint32_t freq, char* path) {

	uint8_t LUT[256];
	uint32_t output_end = TIMER_FREQ / freq;
	uint32_t input_end = 255;

	//72 reaches max extension of PC Speaker, no use having higher values
 	if (output_end >= 72) {
 		output_end = 72;
 	}

	double slope = 1.0 * (output_end) / (input_end);

	//Create lookup table that'll be used to scale the 8-bit PCM data
	size_t i;
	for(i = 0; i < 255; i++) {
		LUT[i] = round(slope * i);
	}

	char* buffer = NULL;
	uint8_t* pcm = NULL;
	size_t size = 0;

	//Open PCM file
	char filename[128];
	strcat(filename, "/home/lcom/lab2/music/");
	strcat(filename, path);
	FILE *fp = fopen(filename, "r");

	if(fp == NULL) {
		printf("Invalid music file!\n");
		return -1;
	}

	//Check how many bytes of PCM we're reading
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);

	//Set seek position to beginning of file
	rewind(fp);

	//Allocate the necessary bytes for the PCM data
	buffer = malloc((size) * sizeof(*buffer));
	pcm = malloc((size) * sizeof(*pcm)); //Duplicate buffer that'll hold the scaled values

	//Read whole file to buffer
	fread(buffer, size, 1, fp);

	//Scale 8-bit values to the range the PC Speaker allows (depends on sample rate but max is 72)
	for(i = 0; i < size; i++) {
		uint8_t sample = buffer[i];
		pcm[i] = LUT[sample];
	}

	free(buffer);

	sys_outb(TIMER_CTRL, 0x90); //mode 0 LSB only, binary

	uint32_t status;
	sys_inb(SPEAKER_CTRL, &status);
	sys_outb(SPEAKER_CTRL, status | BIT(0) | BIT(1)); //Attach Speaker to Timer 2 and extend cone on Timer 2 "high"

	//Subscribe timer interrupts
	int hook = timer_subscribe_int();
	int32_t kbd_irq = kbd_subscribe_int();

	if (hook < 0 || kbd_irq < 0) { //Check if valid hook id
		return -1;
	}
		//Variables for interrupt handling
		int ipc_status, dstatus;
		message msg;

		uint32_t kbd_code = 0;

		int irq_set = BIT(hook);
		kbd_irq = BIT(kbd_irq);

		 //Since function only has time parameter, assuming 60Hz as frequency but code allows any frequency
		if (timer_set_square(0, freq) != 0) {
			timer_unsubscribe_int(); //No need to check since an error is already being returned
			return -1; //Failed to set timer 0 to freq
		}

		//Interrupt handling
		while(counter < size && kbd_code != ESC_BREAK) {
			if ((dstatus = driver_receive(ANY, &msg, &ipc_status)) != 0) {
				printf("driver_receive failed with: %d", dstatus);
				continue;
			}

			//Check if notification
			if (is_ipc_notify(ipc_status)) {
				switch (_ENDPOINT_P(msg.m_source)) {
				case HARDWARE: //Hardware interrupt notification

					//Timer interrupt
					if (msg.NOTIFY_ARG & irq_set) {

						//Call timer 2 with the PCM data
						call_timer2(pcm[counter]);
						counter++;
					}

					//Keyboard interrupt
					if (msg.NOTIFY_ARG & kbd_irq) {

						//Read output port for scancode
						kbd_code = kbd_read();

						//Check scancode read success
						if (kbd_code == KBD_ERROR) {
							kbd_reset(ENABLED);
							sys_inb(SPEAKER_CTRL, &status);
							sys_outb(SPEAKER_CTRL, status & 0xFC);
							timer_unsubscribe_int();
							return -1;
						}
					}

					break;
				default: //No other notifications expected, do nothing
					break;
				}
			}
		}

		//Reset parameters
		counter = 0;
		if (freq != 60) {
			if (timer_set_square(0, 60) != 0) {
				timer_unsubscribe_int(); //No need to check since an error is already being returned
				return -1; //Failed to set timer 0 to 60Hz
			}
		}

		free(pcm);

		//Detach Speaker from Timer 2
		kbd_reset(ENABLED);
		sys_inb(SPEAKER_CTRL, &status);
		sys_outb(SPEAKER_CTRL, status & 0xFC);

		//Unsubscribe timer interrupts
		if (timer_unsubscribe_int() != 0) {
			timer_unsubscribe_int();
			return -1;
		}

	return 0;
}


#define SSIZE_MAX INT_MAX //MINIX defines ssize_t as typedef int ssize_t in types.h
#define _GETDELIM_GROWBY 128    /* amount to grow line buffer by */
#define _GETDELIM_MINLEN 4      /* minimum line buffer size */

//TODO https://gist.github.com/ingramj/1105106
//TODO comment, check
ssize_t getdelim(char** lineptr, size_t* n, int delimiter, FILE* stream) {
	char *buf, *pos;
	int c;
	ssize_t bytes;

	if (lineptr == NULL || n == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (stream == NULL) {
		errno = EBADF;
		return -1;
	}

	/* resize (or allocate) the line buffer if necessary */
	buf = *lineptr;
	if (buf == NULL || *n < _GETDELIM_MINLEN) {
		buf = realloc(*lineptr, _GETDELIM_GROWBY);
		if (buf == NULL) {
			/* ENOMEM */
			return -1;
		}
		*n = _GETDELIM_GROWBY;
		*lineptr = buf;
	}

	/* read characters until delimiter is found, end of file is reached, or an
	   error occurs. */
	bytes = 0;
	pos = buf;
	while ((c = getc(stream)) != EOF) {
		if (bytes + 1 >= SSIZE_MAX) {
			errno = EOVERFLOW;
			return -1;
		}
		bytes++;
		if (bytes >= *n - 1) {
			buf = realloc(*lineptr, *n + _GETDELIM_GROWBY);
			if (buf == NULL) {
				/* ENOMEM */
				return -1;
			}
			*n += _GETDELIM_GROWBY;
			pos = buf + bytes - 1;
			*lineptr = buf;
		}

		if(c == delimiter) {
			break;
		}

		if(c != '\r' && c != '\n') {
			*pos++ = (char) c;
		}

	}

	if (ferror(stream) || (feof(stream) && (bytes == 0))) {
		/* EOF, or an error from getc(). */
		return -1;
	}

	*pos = '\0';
	return bytes;
}


unsigned long parse_ulong(char *str, int base) {
	char *endptr;
	unsigned long val;
	int errsv; //Preserve errno

	//Convert string to unsigned long
	errno = 0; //Clear errno
	val = strtoul(str, &endptr, base);
    errsv = errno;

	//Check for conversion errors
    if (errsv != 0) {
#if defined(DEBUG) && DEBUG == 1
		char *errorstr = strerror(errsv);
		printf("vga: parse_ulong: error %d - %s\n", errsv, errorstr);
#endif
		return ULONG_MAX;
	}

	if (endptr == str) {
#if defined(DEBUG) && DEBUG == 1
		printf("vga: parse_ulong: no digits were found in \"%s\"\n", str);
#endif
		return ULONG_MAX;
	}

	//Successful conversion
	return val;
}




int8_t speaker_square(char* music_path) {

	char filename[128];
	strcat(filename, "/home/lcom/lab2/music/");
	strcat(filename, music_path);
	FILE* musicfile = fopen(filename, "r");

	if(musicfile == NULL) {
		printf("music: file not found\n");
		return -1;
	}

	size_t music_size;

	uint16_t counter = 0;

	char* line = NULL;
	size_t len;
	size_t it = 0;
	ssize_t nread;
	uint16_t flags = 0;

	do{
		//Reads stream up to delim char, terminates line with \0
		nread = getdelim(&line, &len, ',', musicfile);

		if(flags == NOTES_S) {
			music_size = parse_ulong(line, 10);
			notes = malloc((music_size) * sizeof(*notes)); //TODO free
			duration = malloc((music_size) * sizeof(*notes)); //TODO free
			flags = LOOP;
		} else if(flags == DURATION) {

			duration[it] = parse_ulong(line, 10);
			flags = NOTES;
			it++;

			if(it >= music_size) {
				break;
			}

		} else if(flags == 0) {

			//Amount of different notes in music
			if(strncmp(line, "notes", strlen("notes")) == 0) {
				flags = NOTES_S;
			} else {
				printf("music: amount of notes not found on 1st line of file\n");
				return -1;
			}
		} else if(flags == LOOP) {
			loop = parse_ulong(line, 10);
			flags = NOTES;
		} else if(flags == NOTES) {

			//Rest
			if (strncmp(line, "R", strlen("R")) == 0) {
				notes[it] = 0;
			}

			//5th octave
			else if(strncmp(line, "C4", strlen("C4")) == 0) {
				notes[it] = C4;
			} else if(strncmp(line, "Cs4", strlen("Cs4")) == 0 || strncmp(line, "Db4", strlen("Db4")) == 0) {
				notes[it] = CS4;
			} else if(strncmp(line, "D4", strlen("D4")) == 0) {
				notes[it] = D4;
			} else if(strncmp(line, "Ds4", strlen("Ds4")) == 0 || strncmp(line, "Eb4", strlen("Eb4")) == 0)  {
				notes[it] = DS4;
			} else if(strncmp(line, "E4", strlen("E4")) == 0) {
				notes[it] = E4;
			} else if(strncmp(line, "F4", strlen("F4")) == 0) {
				notes[it] = F4;
			} else if(strncmp(line, "Fs4", strlen("Fs4")) == 0 || strncmp(line, "Gb4", strlen("Gb4")) == 0)  {
				notes[it] = FS4;
			} else if(strncmp(line, "G4", strlen("G4")) == 0) {
				notes[it] = G4;
			} else if(strncmp(line, "Gs4", strlen("Gs4")) == 0 || strncmp(line, "Ab4", strlen("Ab4")) == 0)  {
				notes[it] = GS4;
			} else if(strncmp(line, "A4", strlen("A4")) == 0) {
				notes[it] = A4;
			} else if(strncmp(line, "As4", strlen("As4")) == 0 || strncmp(line, "Bb4", strlen("Bb4")) == 0)  {
				notes[it] = AS4;
			} else if(strncmp(line, "B4", strlen("B4")) == 0) {
				notes[it] = B4;
			}

			//6th Octave
			else if(strncmp(line, "C5", strlen("C5")) == 0) {
				notes[it] = C5;
			} else if(strncmp(line, "Cs5", strlen("Cs5")) == 0 || strncmp(line, "Db5", strlen("Db5")) == 0) {
				notes[it] = CS5;
			} else if(strncmp(line, "D5", strlen("D5")) == 0) {
				notes[it] = D5;
			} else if(strncmp(line, "Ds5", strlen("Ds5")) == 0 || strncmp(line, "Eb5", strlen("Eb5")) == 0)  {
				notes[it] = DS5;
			} else if(strncmp(line, "E5", strlen("E5")) == 0) {
				notes[it] = E5;
			} else if(strncmp(line, "F5", strlen("F5")) == 0) {
				notes[it] = F5;
			} else if(strncmp(line, "Fs5", strlen("Fs5")) == 0 || strncmp(line, "Gb5", strlen("Gb5")) == 0)  {
				notes[it] = FS5;
			} else if(strncmp(line, "G5", strlen("G5")) == 0) {
				notes[it] = G5;
			} else if(strncmp(line, "Gs5", strlen("Gs5")) == 0 || strncmp(line, "Ab5", strlen("Ab5")) == 0)  {
				notes[it] = GS5;
			} else if(strncmp(line, "A5", strlen("A5")) == 0) {
				notes[it] = A5;
			} else if(strncmp(line, "As5", strlen("As5")) == 0 || strncmp(line, "Bb5", strlen("Bb5")) == 0)  {
				notes[it] = AS5;
			} else if(strncmp(line, "B5", strlen("B5")) == 0) {
				notes[it] = B5;
			}

			//4th Octave
			else if(strncmp(line, "C3", strlen("C3")) == 0) {
				notes[it] = C3;
			} else if(strncmp(line, "Cs3", strlen("Cs3")) == 0 || strncmp(line, "Db3", strlen("Db3")) == 0) {
				notes[it] = CS3;
			} else if(strncmp(line, "D3", strlen("D3")) == 0) {
				notes[it] = D3;
			} else if(strncmp(line, "Ds3", strlen("Ds3")) == 0 || strncmp(line, "Eb3", strlen("Eb3")) == 0)  {
				notes[it] = DS3;
			} else if(strncmp(line, "E3", strlen("E3")) == 0) {
				notes[it] = E3;
			} else if(strncmp(line, "F3", strlen("F3")) == 0) {
				notes[it] = F3;
			} else if(strncmp(line, "Fs3", strlen("Fs3")) == 0 || strncmp(line, "Gb3", strlen("Gb3")) == 0)  {
				notes[it] = FS3;
			} else if(strncmp(line, "G3", strlen("G3")) == 0) {
				notes[it] = G3;
			} else if(strncmp(line, "Gs3", strlen("Gs3")) == 0 || strncmp(line, "Ab3", strlen("Ab3")) == 0)  {
				notes[it] = GS3;
			} else if(strncmp(line, "A3", strlen("A3")) == 0) {
				notes[it] = A3;
			} else if(strncmp(line, "As3", strlen("As3")) == 0 || strncmp(line, "Bb3", strlen("Bb3")) == 0)  {
				notes[it] = AS3;
			} else if(strncmp(line, "B3", strlen("B3")) == 0) {
				notes[it] = B3;
			}

			//7th Octave
			else if(strncmp(line, "C6", strlen("C6")) == 0) {
				notes[it] = C6;
			} else if(strncmp(line, "Cs6", strlen("Cs6")) == 0 || strncmp(line, "Db6", strlen("Db6")) == 0) {
				notes[it] = CS6;
			} else if(strncmp(line, "D6", strlen("D6")) == 0) {
				notes[it] = D6;
			} else if(strncmp(line, "Ds6", strlen("Ds6")) == 0 || strncmp(line, "Eb6", strlen("Eb6")) == 0)  {
				notes[it] = DS6;
			} else if(strncmp(line, "E6", strlen("E6")) == 0) {
				notes[it] = E6;
			} else if(strncmp(line, "F6", strlen("F6")) == 0) {
				notes[it] = F6;
			} else if(strncmp(line, "Fs6", strlen("Fs6")) == 0 || strncmp(line, "Gb6", strlen("Gb6")) == 0)  {
				notes[it] = FS6;
			} else if(strncmp(line, "G6", strlen("G6")) == 0) {
				notes[it] = G6;
			} else if(strncmp(line, "Gs6", strlen("Gs6")) == 0 || strncmp(line, "Ab6", strlen("Ab6")) == 0)  {
				notes[it] = GS6;
			} else if(strncmp(line, "A6", strlen("A6")) == 0) {
				notes[it] = A6;
			} else if(strncmp(line, "As6", strlen("As6")) == 0 || strncmp(line, "Bb6", strlen("Bb6")) == 0)  {
				notes[it] = AS6;
			} else if(strncmp(line, "B6", strlen("B6")) == 0) {
				notes[it] = B6;
			}

			//3rd Octave
			else if(strncmp(line, "C2", strlen("C2")) == 0) {
				notes[it] = C2;
			} else if(strncmp(line, "Cs2", strlen("Cs2")) == 0 || strncmp(line, "Db2", strlen("Db2")) == 0) {
				notes[it] = CS2;
			} else if(strncmp(line, "D2", strlen("D2")) == 0) {
				notes[it] = D2;
			} else if(strncmp(line, "Ds2", strlen("Ds2")) == 0 || strncmp(line, "Eb2", strlen("Eb2")) == 0)  {
				notes[it] = DS2;
			} else if(strncmp(line, "E2", strlen("E2")) == 0) {
				notes[it] = E2;
			} else if(strncmp(line, "F2", strlen("F2")) == 0) {
				notes[it] = F2;
			} else if(strncmp(line, "Fs2", strlen("Fs2")) == 0 || strncmp(line, "Gb2", strlen("Gb2")) == 0)  {
				notes[it] = FS2;
			} else if(strncmp(line, "G2", strlen("G2")) == 0) {
				notes[it] = G2;
			} else if(strncmp(line, "Gs2", strlen("Gs2")) == 0 || strncmp(line, "Ab2", strlen("Ab2")) == 0)  {
				notes[it] = GS2;
			} else if(strncmp(line, "A2", strlen("A2")) == 0) {
				notes[it] = A2;
			} else if(strncmp(line, "As2", strlen("As2")) == 0 || strncmp(line, "Bb2", strlen("Bb2")) == 0)  {
				notes[it] = AS2;
			} else if(strncmp(line, "B2", strlen("B2")) == 0) {
				notes[it] = B2;
			}

			//8th Octave
			else if(strncmp(line, "C7", strlen("C7")) == 0) {
				notes[it] = C7;
			} else if(strncmp(line, "Cs7", strlen("Cs7")) == 0 || strncmp(line, "Db7", strlen("Db7")) == 0) {
				notes[it] = CS7;
			} else if(strncmp(line, "D7", strlen("D7")) == 0) {
				notes[it] = D7;
			} else if(strncmp(line, "Ds7", strlen("Ds7")) == 0 || strncmp(line, "Eb7", strlen("Eb7")) == 0)  {
				notes[it] = DS7;
			} else if(strncmp(line, "E7", strlen("E7")) == 0) {
				notes[it] = E7;
			} else if(strncmp(line, "F7", strlen("F7")) == 0) {
				notes[it] = F7;
			} else if(strncmp(line, "Fs7", strlen("Fs7")) == 0 || strncmp(line, "Gb7", strlen("Gb7")) == 0)  {
				notes[it] = FS7;
			} else if(strncmp(line, "G7", strlen("G7")) == 0) {
				notes[it] = G7;
			} else if(strncmp(line, "Gs7", strlen("Gs7")) == 0 || strncmp(line, "Ab7", strlen("Ab7")) == 0)  {
				notes[it] = GS7;
			} else if(strncmp(line, "A7", strlen("A7")) == 0) {
				notes[it] = A7;
			} else if(strncmp(line, "As7", strlen("As7")) == 0 || strncmp(line, "Bb7", strlen("Bb7")) == 0)  {
				notes[it] = AS7;
			} else if(strncmp(line, "B7", strlen("B7")) == 0) {
				notes[it] = B7;
			}

			//2nd Octave
			else if(strncmp(line, "C1", strlen("C1")) == 0) {
				notes[it] = C1;
			} else if(strncmp(line, "Cs1", strlen("Cs1")) == 0 || strncmp(line, "Db1", strlen("Db1")) == 0) {
				notes[it] = CS1;
			} else if(strncmp(line, "D1", strlen("D1")) == 0) {
				notes[it] = D1;
			} else if(strncmp(line, "Ds1", strlen("Ds1")) == 0 || strncmp(line, "Eb1", strlen("Eb1")) == 0)  {
				notes[it] = DS1;
			} else if(strncmp(line, "E1", strlen("E1")) == 0) {
				notes[it] = E1;
			} else if(strncmp(line, "F1", strlen("F1")) == 0) {
				notes[it] = F1;
			} else if(strncmp(line, "Fs1", strlen("Fs1")) == 0 || strncmp(line, "Gb1", strlen("Gb1")) == 0)  {
				notes[it] = FS1;
			} else if(strncmp(line, "G1", strlen("G1")) == 0) {
				notes[it] = G1;
			} else if(strncmp(line, "Gs1", strlen("Gs1")) == 0 || strncmp(line, "Ab1", strlen("Ab1")) == 0)  {
				notes[it] = GS1;
			} else if(strncmp(line, "A1", strlen("A1")) == 0) {
				notes[it] = A1;
			} else if(strncmp(line, "As1", strlen("As1")) == 0 || strncmp(line, "Bb1", strlen("Bb1")) == 0)  {
				notes[it] = AS1;
			} else if(strncmp(line, "B1", strlen("B1")) == 0) {
				notes[it] = B1;
			}

			//9th Octave
			else if(strncmp(line, "C8", strlen("C8")) == 0) {
				notes[it] = C8;
			}

			//1st Octave
			else if(strncmp(line, "A0", strlen("A0")) == 0) {
				notes[it] = A0;
			} else if(strncmp(line, "As0", strlen("As0")) == 0 || strncmp(line, "Bb0", strlen("Bb0")) == 0) {
				notes[it] = AS0;
			} else if(strncmp(line, "B0", strlen("B0")) == 0) {
				notes[it] = B0;

			} else {
				printf("it: %d\n", it);
				printf("music: note not recognized\n");
				return -1;
			}

			flags = DURATION;
		} else {
			printf("music: flag not recognized\n");
			return -1;
		}
	} while(nread != -1);

	fclose(musicfile);

//	int i;
//	for(i = 1; i < 10; i++) {
//		printf("%d | %d\n", notes[music_size - i], duration[music_size - i]);
//	}

	test_play(music_size);

	return 0;
}


int8_t test_play(uint16_t amount) {

	sys_outb(TIMER_CTRL, 0xB6);

	unsigned long status;
	sys_inb(SPEAKER_CTRL, &status);
	sys_outb(SPEAKER_CTRL, status | BIT(0) | BIT(1));

	//Subscribe timer interrupts
	int timer_irq = timer_subscribe_int();
	int32_t kbd_irq = kbd_subscribe_int();

	if (timer_irq < 0 || kbd_irq < 0) {
		return -1;
	}

	//Variables for interrupt handling
	int ipc_status, dstatus;
	message msg;
	timer_irq = BIT(timer_irq);
	kbd_irq = BIT(kbd_irq);

	uint16_t it = 0;
	uint32_t wait_frames = 0;
	uint8_t speaker_on = TRUE;
	uint32_t kbd_code = 0;

	//Interrupt handling
	while(kbd_code != ESC_BREAK) {
		if ((dstatus = driver_receive(ANY, &msg, &ipc_status)) != 0) {
			printf("driver_receive failed with: %d", dstatus);
			continue;
		}
		if (is_ipc_notify(ipc_status)) {
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE:

				//Timer interrupts
				if (msg.NOTIFY_ARG & timer_irq) {

					if(it == amount) {
						it = loop;
					}

					if(wait_frames == 0) {

						if(notes[it] == 0) {
							sys_inb(SPEAKER_CTRL, &status);
							sys_outb(SPEAKER_CTRL, status & 0xFC);
							speaker_on = FALSE;
						} else {
							if(speaker_on == FALSE) {
								sys_inb(SPEAKER_CTRL, &status);
								sys_outb(SPEAKER_CTRL, status | BIT(0) | BIT(1));
							}
							timer_set_square(2, notes[it]);
						}

						wait_frames = duration[it];
						it++;
					}

					wait_frames--;
				}

				//Keyboard interrupt
				if (msg.NOTIFY_ARG & kbd_irq) {

					//Read output port for scancode
					kbd_code = kbd_read();

					//Check scancode read success
					if (kbd_code == KBD_ERROR) {
						kbd_reset(ENABLED);
						sys_inb(SPEAKER_CTRL, &status);
						sys_outb(SPEAKER_CTRL, status & 0xFC);
						timer_unsubscribe_int();
						return -1;
					}
				}

				break;

			default:
				break;
			}
		}
	}

	sys_inb(SPEAKER_CTRL, &status);
	sys_outb(SPEAKER_CTRL, status & 0xFC);

	kbd_reset(ENABLED);
	if (timer_unsubscribe_int() != 0) {
		timer_unsubscribe_int();
		return -1;
	}

	printf("Music ended\n");

	return 0;
}
