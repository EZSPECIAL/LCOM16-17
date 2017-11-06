#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/driver.h>
#include "i8254.h"

static int hook_id; //Timer 0 hook id
static int counter = 0; //Interrupt handler counter, global to avoid changing a function that already came defined - void timer_int_handler()

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
    if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &hook_id) == OK) {
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
		if (timer_set_square(0, freq) != 0) {
			timer_unsubscribe_int(); //No need to check since an error is already being returned
			return -1; //Failed to set timer 0 to freq
		}

		//Interrupt handling
		while(counter < freq * time) { //counter is updated at freq/s so we multiply freq * time to get counter value at time (seconds)
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
							timer_int_handler(); //Increments counter
							if (counter % freq == 0) { //counter is updated at freq/s so if the remainder of counter / freq = 0 we are at 1s passed
								printf("Seconds passed: %d\n", counter / freq); //Calculate actual seconds since counter isn't a measure of seconds but interrupts generated
							}
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
		if (freq != 60) {
			if (timer_set_square(0, 60) != 0) {
				timer_unsubscribe_int(); //No need to check since an error is already being returned
				return -1; //Failed to set timer 0 to 60Hz
			}
		}

		//Unsubscribe timer interrupts
		if (timer_unsubscribe_int() == 0) {
			return 0;
		} else return -1; //Failed to unsubscribe timer interrupts
	} else return -1; //Failed to subscribe timer interrupts
}


//Waits time amount of seconds, assumes 60Hz
int timer_sleep(unsigned long time) {

	//Subscribe timer interrupts
	int hook = timer_subscribe_int();
	if (hook >= 0) { //Check if valid hook id

		//Variables for interrupt handling
		int ipc_status, dstatus;
		message msg;
		int irq_set = BIT(hook); //bitmask for timer interrupts
		int freq = 60; //Frequency used by the timer interrupt

		//Interrupt handling
		while(counter < freq * time) { //counter is updated at freq/s so we multiply freq * time to get counter value at time (seconds)
			if ((dstatus = driver_receive(ANY, &msg, &ipc_status)) != 0) { //Receives any messages
				printf("driver_receive failed with: %d", dstatus);
				continue;
			}

			//Check if notification
			if (is_ipc_notify(ipc_status)) {
				switch (_ENDPOINT_P(msg.m_source)) {
					case HARDWARE: //Hardware interrupt notification
						if (msg.NOTIFY_ARG & irq_set) { //Bitmask hook id to check for timer interrupt
							timer_int_handler(); //Increments counter
						}
						break;

					default: //No other notifications expected, do nothing
						break;
				}
			}
		}

		//Reset parameters
		counter = 0;

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
