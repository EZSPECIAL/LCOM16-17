#include <minix/syslib.h>
#include <minix/drivers.h>
#include "i8042.h"
#include "mouse.h"
#include "keyboard.h"

static int mouse_hook; //Mouse hook id

//Tries to return mouse control to MINIX
void mouse_reset(int data_report) {

	int timeout = TIMEOUT;

	if(data_report) {
		while(timeout > 0) {
			if(mouse_command(DISABLE_DATA) != 0) {
				//mouse_discard();
				timeout--;
			} else break;
		}
	}

	timeout = TIMEOUT;

	while(timeout > 0) {
		if (mouse_unsubscribe_int() == 0)
			break;
		timeout--;
	}
	mouse_discard(); //Discard leftover output so MINIX doesn't lockup
}

//Writes command to specified port
//Returns 0 on success, KBD_ERROR otherwise
int mouse_write(unsigned long command, int port) {

	int timeout = TIMEOUT;
	unsigned long status, data;

	//Retry "timeout" number of times
	while(timeout > 0) {
		if(sys_inb(STATUS_PORT, &status) != OK) {
			printf("mouse: mouse_write: failed to read PS/2 controller status byte\n");
			return KBD_ERROR;
		}

		//Since responses are always read in mouse_command() we can probably assume this is a rogue output,
		//discarding it since it might crash the program
		if(status & OUT_BUF_STATUS) {
			if(sys_inb(KBD_OUT_BUF, &data) != OK) {
				printf("mouse: mouse_write failed to read kbd data\n");
				return KBD_ERROR;
			}
		}

		//Check if status register is ready for input
		if((status & IN_BUF_STATUS) == 0) {
			if(sys_outb(port, command) != OK) {
				printf("mouse: mouse_write: failed to write command to register\n");
				return KBD_ERROR;
			}

			delay(1); //Wait 20ms since we could be calling this function immediately after
			return 0;
		}

		delay(1); //Wait 20ms between tries
		timeout--;
	}

	printf("mouse: mouse_write: timed out\n");
	return KBD_ERROR;
}


//Reads command from PS/2 controller output port
//Returns data read on success, KBD_ERROR otherwise
int mouse_read() {
	int timeout = TIMEOUT;
	unsigned long status, data;

	//Retry "timeout" number of times
	while(timeout > 0) {
		if(sys_inb(STATUS_PORT, &status) != OK) {
			printf("mouse: mouse_read: failed to read PS/2 controller status byte\n");
			return KBD_ERROR;
		}

		//Check if output buffer has data for us to read
		if(status & OUT_BUF_STATUS) {
			if(sys_inb(KBD_OUT_BUF, &data) != OK) {
				printf("mouse: mouse_read: failed to read kbd data\n");
				return KBD_ERROR;
			}

			//Check if serial communication between device and PS/2 controller succeeded
			if((status & (PARITY_ERROR | TIME_OUT_ERROR)) == 0) {
				delay(1); //Wait 20ms since we could be calling this function immediately after
				return data;
			} else {
				printf("mouse: mouse_read: parity and/or time-out error\n");
				return KBD_ERROR;
			}
		}

		delay(1); //Wait 20ms between tries
		timeout--;
	}

	printf("mouse: mouse_read: timed out\n");
	return KBD_ERROR;
}


//Reads command from PS/2 controller output port but discards it
int mouse_discard() {
	int timeout = TIMEOUT;
	unsigned long status, data;

	//Retry "timeout" number of times
	while(timeout > 0) {
		if(sys_inb(STATUS_PORT, &status) != OK) {
			printf("mouse: mouse_read: failed to read PS/2 controller status byte\n");
			return -1;
		}

		//Check if output buffer has data for us to read
		if(status & OUT_BUF_STATUS) {
			if(sys_inb(KBD_OUT_BUF, &data) != OK) {
				printf("mouse: mouse_read: failed to read kbd data\n");
				return -1;
			}
		}

		delay(1); //Wait 20ms between tries
		timeout--;
	}

	printf("mouse: mouse_read: timed out\n");
	return 0;
}


//Writes command to port, does so by calling mouse_write followed by mouse_read for response
//Verifies response from PS/2 controller, resends if needed or returns
//Returns 0 on success, -1 otherwise
int mouse_command(unsigned long command) {

	unsigned long response;

	//Do once unless response was 0xFE (RESEND)
	do {

		//Write command to specified port
		if(mouse_write(SECOND_PS2_WRITE, KBD_CMD_BUF) == KBD_ERROR) {
			printf("mouse: mouse_command: error sending command to controller\n");
			return -1;
		}

		//Write command to specified port
		if(mouse_write(command, KBD_IN_BUF) == KBD_ERROR) {
			printf("mouse: mouse_command: error sending command\n");
			return -1;
		}

		response = mouse_read(); //Get response from the controller

	#if defined(DEBUG) && DEBUG == 1
		printf("Response: 0x%02X\n", response);
	#endif

		//mouse_read() returns KBD_ERROR in case it couldn't read data
		if(response == KBD_ERROR) {
			printf("mouse: mouse_command: error reading response\n");
			return -1;
		}

	} while(response == KBD_RESEND || response == KBD_FAIL);

	//Checks if command was acknowledged by the controller
	if (response == KBD_ACK) {
		return 0;
	} else {
		printf("mouse: mouse_command: unrecognized response from controller\n");
		return -1;
	}
}


//Subscribes mouse interrupts
//Returns hook_id bit position on success, -1 otherwise
int mouse_subscribe_int() {
	mouse_hook = MOUSE_HOOK_BIT;
    if (sys_irqsetpolicy(MOUSE_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &mouse_hook) == OK) { //Additionally call IRQ_EXCLUSIVE so MINIX doesn't use its own IH
    	if (sys_irqenable(&mouse_hook) == OK) {
#if defined(DEBUG) && DEBUG == 1
    		printf("Hook id returned: 0x%02X\n\n", mouse_hook);
#endif
    		return MOUSE_HOOK_BIT;
    	} else {
    		printf("mouse: mouse_subcribe_int: failed to enable interrupts\n");
    		return -1;
    	}
    } else {
    	printf("mouse: mouse_subscribe_int: failed to set policy\n");
    	return -1;
    }
}


//Unsubscribe mouse interrupts
//Returns 0 on success, -1 otherwise
int mouse_unsubscribe_int() {
    if(sys_irqdisable(&mouse_hook) == OK) {
    	if(sys_irqrmpolicy(&mouse_hook) == OK) {
    		return 0;
    	} else {
    		printf("mouse: mouse_unsubscribe_int: failed to disable policy\n");
    		return -1;
    	}
    } else {
    		printf("mouse: mouse_unsubscribe_int: failed to disable interrupts\n");
    		return -1;
    }
}


int8_t mouse_magic_sequence() {

	uint8_t id;
	mouse_command(MOUSE_ID);
	id = mouse_read();

	mouse_command(SAMPLE_RATE);
	mouse_command(MOUSE_SAMPLE200);
	mouse_command(SAMPLE_RATE);
	mouse_command(MOUSE_SAMPLE100);
	mouse_command(SAMPLE_RATE);
	mouse_command(MOUSE_SAMPLE80);
	mouse_command(MOUSE_ID);

	id = mouse_read();

	printf("id: %d\n", id);

	if(id == 3) {
		mouse_command(SAMPLE_RATE);
		mouse_command(MOUSE_SAMPLE200);
		mouse_command(SAMPLE_RATE);
		mouse_command(MOUSE_SAMPLE200);
		mouse_command(SAMPLE_RATE);
		mouse_command(MOUSE_SAMPLE80);
		mouse_command(MOUSE_ID);

		id = mouse_read();

		printf("id: %d\n", id);
	} else return id;

	mouse_command(SAMPLE_RATE);
	mouse_command(MOUSE_SAMPLE100);

	return id;
}
