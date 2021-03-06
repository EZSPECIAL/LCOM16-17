#include <minix/syslib.h>
#include <minix/drivers.h>
#include "i8042.h"
#include "i8254.h"
#include "timer.h"
#include "keyboard.h"
#include "video.h"

static int kbd_hook; //Keyboard hook id

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
	if (!scancode_status) {
		kbd_command(KBD_ENABLE, KBD_IN_BUF); //Scancodes where disabled by some function, try to re-enable
	}
	int timeout = TIMEOUT;
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

		//Check if output buffer has data for us to read
		if(status & OUT_BUF_STATUS) {
			if(sys_inb(KBD_OUT_BUF, &data) != OK) {
				printf("keyboard: kbd_read: failed to read kbd data\n");
				return KBD_ERROR;
			}

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

	printf("keyboard: kbd_read: timed out\n");
	return 0;
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
}
