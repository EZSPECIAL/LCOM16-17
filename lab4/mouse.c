#include <minix/syslib.h>
#include <minix/drivers.h>
#include "i8042.h"
#include "mouse.h"

int mouse_hook; //Mouse hook id
static int counter = 0;

static state_t state = INIT;

//Sleep 20ms*cycles for i8042 interfacing
//Uses tickdelay from MINIX
void delay(int cycles) {
	while (cycles > 0) {
		tickdelay(micros_to_ticks(DELAY_US));
		cycles--;
	}
}

//Tries to return mouse control to MINIX
void mouse_reset(int data_report) {

	int timeout = TIMEOUT;

	if(data_report) {
		while(timeout > 0) {
			if(mouse_command(DISABLE_DATA) != 0) {
				mouse_discard();
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
	unsigned long data;

	//Retry "timeout" number of times
	while(timeout > 0) {
		if(sys_inb(KBD_OUT_BUF, &data) != OK) {
			printf("mouse: mouse_discard: failed to read kbd data\n");
			return -1;
		}
		delay(1); //Wait 20ms between tries
		timeout--;
	}

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


//Prints mouse packets in user friendly way
//Returns 0 on success, -1 otherwise
int mouse_print_packet(unsigned char* packet) {

	printf("B1: 0x%02X ", packet[0]);
	printf("B2: 0x%02X ", packet[1]);
	printf("B3: 0x%02X ", packet[2]);
	printf("LB: %d ", packet[0] & MOUSE_LB);
	printf("MB: %d ", (packet[0] & MOUSE_MB) >> 2);
	printf("RB: %d ", (packet[0] & MOUSE_RB) >> 1);
	printf("XOV: %d ", (packet[0] & MOUSE_XOV) >> 6);
	printf("YOV: %d ", (packet[0] & MOUSE_YOV) >> 7);

	if((packet[0] & MOUSE_XS) == MOUSE_XS) {
		printf("X: -%3u ", (~packet[1] + 1) & 0xFF);
	} else printf("X: %3u  ", packet[1]);
	if((packet[0] & MOUSE_YS) == MOUSE_YS) {
		printf("Y: -%3u", (~packet[2] + 1) & 0xFF);
	} else printf("Y: %3u", packet[2]);
	printf("\n");
	return 0;
}

//Interrupt handler for mouse interrupts
//cnt - packet count limit
//Returns 0 on success, -1 otherwise
int mouse_int_handler(int mouse_irq, unsigned short cnt) {

	//Variables for interrupt handling
	int ipc_status, dstatus;
	message msg;
	unsigned char packet[3] = {0, 0, 0};
	unsigned short pnumber = 0;
	unsigned short sync = 0;
	unsigned long data;

	//Handle interrupts until packet limit
	while(cnt > 0) {
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
					if (msg.NOTIFY_ARG & mouse_irq) { //Mouse interrupt

						//Read output port for scancode
						if(sys_inb(KBD_OUT_BUF, &data) != OK) {
							printf("mouse: mouse_int_handler: error reading from output buffer\n");
							return -1;
						}

						//Reset pnumber if needed
						if(pnumber == 3) {
							pnumber = 0;
						}

						packet[pnumber] = data;

						//Skips 1st packet since packet[1] & SYNC_BIT will always be 0 on the 1st pass
						//Useful because ACK (0xFA) or ENTER break code 0x9C gets read as a packet sometimes
						//Possible MINIX kbd driver interference
						pnumber++;

						if((packet[pnumber] & SYNC_BIT) && sync == 0) {
							pnumber = 0;
							sync = 1;
						}

						if((packet[0] & SYNC_BIT) == 0) {
							sync = 0;
						}

						if(pnumber == 3 && sync == 1) {
							cnt--;
							mouse_print_packet(packet);
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

	return 0;
}


//Interrupt handler for timer/mouse interrupts, stops after "time" seconds
//Returns 0 on success, -1 otherwise
int mouse_timed_handler(int mouse_irq, int timer_irq, unsigned short time) {

			//Variables for interrupt handling
			int ipc_status, dstatus;
			message msg;
			int freq = 60; //Frequency used by the timer, assuming 60Hz
			unsigned char packet[3] = {0, 0, 0};
			unsigned short pnumber = 0;
			unsigned short sync = 0;
			unsigned long data;

			//Handle interrupts unti "time" seconds have passed or until user depresses ESC key
			while(counter < freq * time) {
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

							if (msg.NOTIFY_ARG & mouse_irq) { //Keyboard interrupt

								//Read output port for scancode
								if(sys_inb(KBD_OUT_BUF, &data) != OK) {
									printf("mouse: mouse_int_handler: error reading from output buffer\n");
									return -1;
								}

								//Reset pnumber if needed and counter

								counter = 0;

								if(pnumber == 3) {
									pnumber = 0;
								}

								packet[pnumber] = data;

								//Skips 1st packet since packet[1] & SYNC_BIT will always be 0 on the 1st pass
								//Useful because ACK (0xFA) or ENTER break code 0x9C gets read as a packet sometimes
								//Possible MINIX kbd driver interference
								pnumber++;

								if((packet[pnumber] & SYNC_BIT) && sync == 0) {
									pnumber = 0;
									sync = 1;
								}

								if((packet[0] & SYNC_BIT) == 0) {
									sync = 0;
								}

								if(pnumber == 3 && sync == 1) {
									mouse_print_packet(packet);
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

			//Reset parameters
			counter = 0;

			return 0;
}


//Prints mouse config
int config_print(unsigned char* config) {
	if((config[0] & MOUSE_MODE) == MOUSE_MODE) {
		printf("Mouse mode: remote\n");
	} else printf("Mouse mode: stream\n");

	if((config[0] & MOUSE_ENABLED) == MOUSE_ENABLED) {
		printf("Data reporting: enabled\n");
	} else printf("Data reporting: disabled\n");

	if((config[0] & MOUSE_SCALING) == MOUSE_SCALING) {
		printf("Scaling: 2:1\n");
	} else printf("Scaling 1:1\n");

	printf("Resolution: %d\n", BIT(config[1]));

	printf("Sample Rate: %d\n", config[2]);
	return 0;
}

//Reads mouse config bytes and calls config_print
int mouse_config() {
	unsigned char config[3];
	unsigned long data;
	int i;

	for(i = 0; i < 3; i++) {
		if (sys_inb(KBD_OUT_BUF, &data) != OK) {
			return -1;
		}
		config[i] = data;
	}

	config_print(config);
	return 0;
}

//State machine for test_gesture
void mouse_state(ev_type_t event) {

	switch (state) {
		case INIT:
			if(event == RDOWN)
				state = DRAW;
			break;
		case DRAW:
			if(event == GESTURE) {
				state = GOAL;
			} else if(event == RUP)
				state = INIT;
			break;
		default:
			break;
	}
}


//Interrupt handler for mouse interrupts, exist upon gesture
//length - movement in Y direction needed to exit
//Returns 0 on success, -1 otherwise
int mouse_gesture_handler(int mouse_irq, short length) {

	//Variables for interrupt handling
	int ipc_status, dstatus;
	message msg;
	unsigned char packet[3] = {0, 0, 0};
	unsigned short pnumber = 0;
	unsigned short sync = 0;
	unsigned long data;

	short totalY = 0;

	//Handle interrupts until packet limit
	while(state != GOAL) {
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
					if (msg.NOTIFY_ARG & mouse_irq) { //Mouse interrupt

						//Read output port for scancode
						if(sys_inb(KBD_OUT_BUF, &data) != OK) {
							printf("mouse: mouse_int_handler: error reading from output buffer\n");
							return -1;
						}

						//Reset pnumber if needed
						if(pnumber == 3) {
							pnumber = 0;
						}

						packet[pnumber] = data;

						//Skips 1st packet since packet[1] & SYNC_BIT will always be 0 on the 1st pass
						//Useful because ACK (0xFA) or ENTER break code 0x9C gets read as a packet sometimes
						//Possible MINIX kbd driver interference
						pnumber++;

						if((packet[pnumber] & SYNC_BIT) && sync == 0) {
							pnumber = 0;
							sync = 1;
						}

						if((packet[0] & SYNC_BIT) == 0) {
							sync = 0;
						}

						if(pnumber == 3 && sync == 1) {
							mouse_print_packet(packet);

							//Discard if overflow happened, gets rid of some weird bytes too
							if(!((packet[0] & MOUSE_XOV) == MOUSE_XOV || (packet[0] & MOUSE_YOV) == MOUSE_YOV)) {

								//Check if right mouse button is being held and change state
								if((packet[0] & MOUSE_RB) == MOUSE_RB) {
									mouse_state(RDOWN);
								} else {
									totalY = 0;
									mouse_state(RUP);
								}

								//In drawing state, right mouse button is being held
								if(state == DRAW) {

									//Check if X movement is in the right direction, left if length is negative or right if length is positive (positive slope)
									if(((packet[0] & MOUSE_XS) == MOUSE_XS && length < 0) || ((packet[0] & MOUSE_XS) == 0 && length > 0)) {

										//Accumulate Y value if direction of movement has the same sign as length
										if((packet[0] & MOUSE_YS) == MOUSE_YS && length < 0) {
											totalY -= ((~packet[2] + 1) & 0xFF);
										} else if((packet[0] & MOUSE_YS) == 0 && length > 0) {
											totalY += packet[2];

										//Y movement was in opposite direction, reset if movement was larger than tolerance
										} else if((packet[0] & MOUSE_YS) == MOUSE_YS) {
											if(((~packet[2] + 1) & 0xFF) > TOLERANCE)
												totalY = 0;
										} else if((packet[0] & MOUSE_YS) == 0) {
											if(packet[2] > TOLERANCE)
												totalY = 0;
										}

										//Check if length has been reached
										if(length > 0) {
											if(totalY > length)
												mouse_state(GESTURE);
										} else if(length > totalY) {
											mouse_state(GESTURE);
										}
									} else {

										//X movement was in opposite direction, reset if movement was larger than tolerance
										if((packet[0] & MOUSE_XS) == MOUSE_XS) {
											if(((~packet[1] + 1) & 0xFF) > TOLERANCE)
												totalY = 0;
										} else if((packet[0] & MOUSE_XS) == 0) {
											if(packet[1] > TOLERANCE)
												totalY = 0;
										}
									}
								}
							}
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

	return 0;
}
