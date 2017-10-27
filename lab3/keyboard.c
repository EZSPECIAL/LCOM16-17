#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/driver.h>
#include "i8042.h"
#include "i8254.h"
#include "timer.h"
#include "keyboard.h"

static int extended = 0;
static int counter = 0;
static int sys_inb_count = 0;
int kbd_hook; //Keyboard hook id

//Sleep 20ms*cycles for i8042 interfacing
//Uses tickdelay from MINIX
void delay(int cycles) {
	while (cycles > 0) {
		tickdelay(micros_to_ticks(DELAY_US));
		cycles--;
	}
}


//Tries to return keyboard control to MINIX, scancode_status defines whether scancodes should be re-enabled
void kbd_reset(int scancode_status) {

	//Re-enable scancodes if they were disabled
	if (!scancode_status) {
		kbd_command(KBD_ENABLE, KBD_IN_BUF);
	}

	int timeout = TIMEOUT;
	int response = 0;

	printf("keyboard: kbd_reset: attempting to reset command byte, ignore errors.\n");

	//Read current command byte to restore
	while(timeout > 0) {

		response = kbd_command(KBC_READ_CMD, KBD_CMD_BUF);
		if(response != -1) break;
		timeout--;
	}

	if(response != -1) {

		timeout = TIMEOUT;

		//Write command byte with bit 0 on so interrupts are enabled
		while(timeout > 0) {

			if(kbd_command(KBC_WRITE_CMD, KBD_CMD_BUF) != 0) {
				timeout--;
				continue;
			}

			if(kbd_write(response | BIT(0), KBD_IN_BUF) != KBD_ERROR) break;
			timeout--;
		}
	}

	timeout = TIMEOUT;

	printf("keyboard: kbd_reset: forcing unsubscribe, ignore errors.\n");

	//Try to force unsubscribing
	while(timeout > 0) {
		if (kbd_unsubscribe_int() == 0)
			break;
		timeout--;
	}

	kbd_discard(); //Discard leftover output so MINIX doesn't lockup
}


//Subscribes keyboard interrupts
//Returns hook_id bit position on success, -1 otherwise
int kbd_subscribe_int() {
	kbd_hook = KBD_HOOK_BIT;
    if (sys_irqsetpolicy(KBD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &kbd_hook) == OK) { //Additionally call IRQ_EXCLUSIVE so MINIX doesn't use its own IH
    	if (sys_irqenable(&kbd_hook) == OK) {
#if defined(DEBUG) && DEBUG == 1
    		printf("Hook id returned: 0x%02X\n\n", kbd_hook);
#endif
    		return KBD_HOOK_BIT;
    	} else {
    		printf("keyboard: keyboard_subcribe_int: failed to enable interrupts\n");
    		return -1;
    	}
    } else {
    	printf("keyboard: keyboard_subscribe_int: failed to set policy\n");
    	return -1;
    }
}


//Unsubscribe keyboard interrupts
//Returns 0 on success, -1 otherwise
int kbd_unsubscribe_int() {
    if(sys_irqdisable(&kbd_hook) == OK) {
    	if(sys_irqrmpolicy(&kbd_hook) == OK) {
    		return 0;
    	} else {
    		printf("keyboard: keyboard_unsubscribe_int: failed to disable policy\n");
    		return -1;
    	}
    } else {
    		printf("keyboard: keyboard_unsubscribe_int: failed to disable interrupts\n");
    		return -1;
    }
}


//Writes command to specified port
//Returns 0 on success, KBD_ERROR otherwise
int kbd_write(unsigned long command, int port) {

	int timeout = TIMEOUT;
	unsigned long status, data;

	//Retry "timeout" number of times
	while(timeout > 0) {
		if(sys_inb(STATUS_PORT, &status) != OK) {
			printf("keyboard: kbd_write: failed to read PS/2 controller status byte\n");
			return KBD_ERROR;
		}

		//Since responses are always read in kbd_command() we can probably assume this is a rogue scancode,
		//discarding it since it crashes the program whether we accept it as response or not
		if(status & OUT_BUF_STATUS) {
			if(sys_inb(KBD_OUT_BUF, &data) != OK) {
				printf("keyboard: kbd_read failed to read kbd data\n");
				return KBD_ERROR;
			}
		}

		//Check if status register is ready for input
		if((status & IN_BUF_STATUS) == 0) {
			if(sys_outb(port, command) != OK) {
				printf("keyboard: kbd_write: failed to write command to register\n");
				return KBD_ERROR;
			}

			delay(1); //Wait 20ms since we could be calling this function immediately after
			return 0;
		}

		delay(1); //Wait 20ms between tries
		timeout--;
	}

	printf("keyboard: kbd_write: timed out\n");
	return KBD_ERROR;
}


//Reads command from PS/2 controller output port
//Returns data read on success, KBD_ERROR otherwise
int kbd_read() {
	int timeout = TIMEOUT;
	unsigned long status, data;

	//Retry "timeout" number of times
	while(timeout > 0) {

		if(sys_inb(STATUS_PORT, &status) != OK) {
			printf("keyboard: kbd_read: failed to read PS/2 controller status byte\n");
			return KBD_ERROR;
		}

		sys_inb_count++;

		//Check if output buffer has data for us to read
		if(status & OUT_BUF_STATUS) {
			if(sys_inb(KBD_OUT_BUF, &data) != OK) {
				printf("keyboard: kbd_read: failed to read kbd data\n");
				return KBD_ERROR;
			}

			sys_inb_count++;

			//Check if serial communication between device and PS/2 controller succeeded
			if((status & (PARITY_ERROR | TIME_OUT_ERROR)) == 0) {
				delay(1); //Wait 20ms since we could be calling this function immediately after
				return data;
			} else {
				printf("keyboard: kbd_read: parity and/or time-out error\n");
				return KBD_ERROR;
			}
		}

		delay(1); //Wait 20ms between tries
		timeout--;
	}

#if defined(DEBUG) && DEBUG == 1
	printf("keyboard: kbd_read: timed out\n");
#endif
	return KBD_ERROR;
}


//Reads command from PS/2 controller output port but discards it
int kbd_discard() {
	int timeout = TIMEOUT;
	unsigned long data;

	//Retry "timeout" number of times
	while(timeout > 0) {
		if(sys_inb(KBD_OUT_BUF, &data) != OK) {
			printf("keyboard: kbd_read: failed to read kbd data\n");
			return -1;
		}
		delay(1); //Wait 20ms between tries
		timeout--;
	}

	return 0;
}


//Writes command to port, does so by calling kbd_write followed by kbd_read for response
//Verifies response from PS/2 controller, resends if needed or returns
//Returns 0 on success, -1 otherwise
int kbd_command(unsigned long command, int port) {

	unsigned long response;

	if(port == KBD_IN_BUF) {

		//Do once unless response was 0xFE (RESEND)
		do {

			//Write command to specified port
			if(kbd_write(command, port) == KBD_ERROR) {
				printf("keyboard: kbd_command: error sending command\n");
				return -1;
			}

			response = kbd_read(); //Get response from the controller

		#if defined(DEBUG) && DEBUG == 1
			printf("Response: 0x%02X\n", response);
		#endif

			//kbd_read() returns KBD_ERROR in case it couldn't read data
			if(response == KBD_ERROR) {
				printf("keyboard: kbd_command: error reading response\n");
				return -1;
			}

		} while(response == KBD_RESEND);

		//Checks if command was acknowledged by the controller
		if (response == KBD_ACK) {
			return 0;
		} else {
			printf("keyboard: kbd_command: unrecognized response from controller\n");
			return -1;
		}
	} else if(port == KBD_CMD_BUF) {

		//Write command to specified port
		if(kbd_write(command, port) == KBD_ERROR) {
			printf("keyboard: kbd_command: error sending command\n");
			return -1;
		}

		if(command == KBC_WRITE_CMD) return 0;

		response = kbd_read(); //Get response from the controller

	#if defined(DEBUG) && DEBUG == 1
		printf("Response: 0x%02X\n", response);
	#endif

		//kbd_read() returns KBD_ERROR in case it couldn't read data
		if(response == KBD_ERROR) {
			printf("keyboard: kbd_command: error reading response\n");
			return -1;
		}

		return response;
	} else return -1;
}


//Prints scancode received from keyboard
//Always returns 0
int kbd_scancode_print(unsigned long scancode) {

	//If previous scancode was 0xE0 (TWOB_CODE) then we're ready to print 2 Byte scancode
	if(extended) {
		extended = 0;
		if (scancode & BIT(7)) {
			printf("Break: 0xE0 0x%02X\n", scancode);
			return 0;
		} else {
			printf("Make: 0xE0 0x%02X\n", scancode);
			return 0;
		}
	//Check whether scancode is the start of a 2 Byte scancode or a 1 Byte one
	} else if (scancode == TWOB_CODE) {
		extended = 1;
		return 0;
	} else if (scancode & BIT(7)) {
		printf("Break: 0x%02X\n", scancode);
		return 0;
	} else {
		printf("Make: 0x%02X\n", scancode);
		return 0;
	}
}


//Interrupt handler for keyboard interrupts
//ass - defines whether to use C or asm handler
//Returns 0 on success, -1 otherwise
int kbd_int_handler(int kbd_irq, unsigned short ass) {

	//Variables for interrupt handling
	int ipc_status, dstatus;
	message msg;
	unsigned long kbd_code;
	short first = 1; //Toggle for the 1st scancode received

	//Handle interrupts until user depresses ESC key
	while(kbd_code != ESC_BREAK) {
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
					if (msg.NOTIFY_ARG & kbd_irq) { //Keyboard interrupt

						//Read output port for scancode
						if (ass) {
							kbd_code = kbd_asm_handler();
						} else kbd_code = kbd_read();

						//Check scancode read success
						if (kbd_code == KBD_ERROR) {
							if (ass) //kbd_read already prints errors, only the asm handler doesn't
								printf("keyboard: kbd_test_scan: scancode reading returned error\n");
							kbd_reset(ENABLED);
							return -1;
						}

						//Prints scancode, 1 or 2 Byte scancodes, PrintScr and Pause/Break are not handled
						if(!first) { //Skip first scancode since it's the break code from calling the program
							kbd_scancode_print(kbd_code);
						} else first = 0;
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

	return 0;
}


//Interrupt handler for timer/keyboard interrupts, stops after "time" seconds
//Returns 0 on success, -1 otherwise
int kbd_timed_handler(int kbd_irq, int timer_irq, unsigned short time) {

			//Variables for interrupt handling
			int ipc_status, dstatus;
			message msg;
			int freq = 60; //Frequency used by the timer, assuming 60Hz
			unsigned long kbd_code;
			short first = 1; //Toggle for the 1st scancode received

			//Handle interrupts unti "time" seconds have passed or until user depresses ESC key
			while(counter < freq * time && kbd_code != ESC_BREAK) {
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
							if (msg.NOTIFY_ARG & timer_irq) { //Timer interrupt
								counter++;
							}

							if (msg.NOTIFY_ARG & kbd_irq) { //Keyboard interrupt
								counter = 0;

								//Read output port for scancode
								kbd_code = kbd_read();

								//Check scancode read success
								if (kbd_code == KBD_ERROR) {
									kbd_reset(ENABLED);
									return -1;
								}

								//Prints scancode, 1 or 2 Byte scancodes, PrintScr and Pause/Break are not handled
								if(!first) { //Skip first scancode since it's the break code from calling the program
									kbd_scancode_print(kbd_code);
								} else first = 0;
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

			//Reset parameters
			counter = 0;

			return 0;
}

int kbd_poll() {

	unsigned long kbd_code = 0;
	sys_inb_count = 0;

	while(kbd_code != ESC_BREAK) {

		kbd_code = kbd_read();
		if(kbd_code != KBD_ERROR) kbd_scancode_print(kbd_code);
	}

	printf("sys_inb total calls: %d\n", sys_inb_count);

	//Read KBC command byte so we can preserve it
	int response = kbd_command(KBC_READ_CMD, KBD_CMD_BUF);

	if(response == -1) return -1;

	//Tell KBC we're writing a new command byte, then write it
	if(kbd_command(KBC_WRITE_CMD, KBD_CMD_BUF) != 0) return -1;
	if(kbd_write(response | BIT(0), KBD_IN_BUF) == KBD_ERROR) return -1;

	return 0;
}
