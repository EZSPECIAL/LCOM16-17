#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/types.h>

#include "UART.h"
#include "keyboard.h"
#include "i8042.h"

static int proc_args(int argc, char **argv);
static unsigned long parse_ulong(char *str, int base);
static void print_usage(char **argv);
static int8_t test_send();
static int8_t test_receive();

int main(int argc, char **argv) {
	sef_startup();

	//Prints usage of the program if no arguments are passed
	if (argc == 1) {
		print_usage(argv);
		return 0;
	}
	else return proc_args(argc, argv);
}


//Print program usage
static void print_usage(char **argv)
{
	printf("Usage:\n"
			"       service run %s -args \"send\"\n"
			"       service run %s -args \"receive\"\n",
			argv[0], argv[0]);
}


//Process arguments passed from user by MINIX terminal
static int proc_args(int argc, char **argv)
{

	//test_send()
	if (strncmp(argv[1], "send", strlen("send")) == 0) {
		if (argc != 2) {
			printf("serial: wrong number of arguments for test_send()\n");
			return 1;
		}

		printf("serial::test_send()\n");
		return test_send();
	}

	//test_receive()
	else if (strncmp(argv[1], "receive", strlen("receive")) == 0) {
		if (argc != 2) {
			printf("serial: wrong number of arguments for test_receive()\n");
			return 1;
		}

		printf("serial::test_receive()\n");
		return test_receive();
	} else {
		printf("serial: %s - not a valid function!\n", argv[1]);
		return 1;
	}
}


static unsigned long parse_ulong(char *str, int base)
{
	char *endptr;
	unsigned long val;
	int errsv; //Preserve errno

	//Convert string to unsigned long
	errno = 0; //Clear errno
	val = strtoul(str, &endptr, base);
    errsv = errno;

	//Check for conversion errors
    if (errsv != 0) {
		return ULONG_MAX;
	}

	if (endptr == str) {
		return ULONG_MAX;
	}

	//Successful conversion
	return val;
}


static int8_t test_send() {

	UART_config_t config;

	uart_set_conf(COM1_BASE, transmit8N1_9600);
	//uart_get_config(COM1_BASE, &config);
	//uart_disp_conf(config);

	int32_t kbd_irq = kbd_subscribe_int();

	if(kbd_irq < 0) {
		printf("lab7: failed to subscribe keyboard interrupts\n");
		return -1;
	}

	kbd_irq = BIT(kbd_irq);

	int ipc_status, dstatus;
	message msg;
	uint32_t kbd_code;

	//Handle interrupts until user depresses ESC key
	while(kbd_code != ESC_BREAK) {
		if ((dstatus = driver_receive(ANY, &msg, &ipc_status)) != 0) { //Receives any messages
			printf("driver_receive failed with: %d", dstatus);
			continue;
		}

		//Check if notification
		if (is_ipc_notify(ipc_status)) {
			switch (_ENDPOINT_P(msg.m_source)) {
				case HARDWARE: //Hardware interrupt notification

					//Keyboard interrupt
					if (msg.NOTIFY_ARG & kbd_irq) {

						//Read output port for scancode
						kbd_code = kbd_read();

						//Check scancode read success
						if (kbd_code == KBD_ERROR) {
							kbd_reset(ENABLED);
							uart_reset(COM1_BASE);
							return -1;
						}

						uart_send(kbd_code);
					}
					break;

				default:
					break;
			}
		}
	}

	kbd_reset(ENABLED);
	uart_reset(COM1_BASE);

	printf("Ended\n");
	return 0;
}


static int8_t test_receive() {

	UART_config_t config;

	uart_set_conf(COM1_BASE, receive8N1_9600);
	//uart_get_config(COM1_BASE, &config);
	//uart_disp_conf(config);

	uint32_t com1_irq = uart_subscribe();

	if(com1_irq < 0) {
		return -1;
	}

	//uint32_t received;
	//uint32_t lsr_status, iir_status;
	uint8_t endflag = FALSE;
	uint32_t serial_rcv = 0;

	int ipc_status, dstatus;
	message msg;
	com1_irq = BIT(com1_irq);

	while(endflag == FALSE) {
		if ((dstatus = driver_receive(ANY, &msg, &ipc_status)) != 0) {
			printf("driver_receive failed with: %d", dstatus);
			continue;
		}

		//Check if notification
		if (is_ipc_notify(ipc_status)) {
		//Debug info for notifications and interrupt bitmask
			switch (_ENDPOINT_P(msg.m_source)) {
				case HARDWARE: //Hardware interrupt notification

					//COM1 Interrupt
					if (msg.NOTIFY_ARG & com1_irq) {

						serial_rcv = uart_receive();

						if(serial_rcv == UART_ERROR) {
							uart_unsubscribe();
							uart_reset(COM1_BASE);
							return -1;
						} else if(serial_rcv == RCV_ERROR) {
							printf("COM1: error in transmission\n");
						} else {
							if(serial_rcv == 0x01) {
								endflag = TRUE;
							}

							printf("0x%02X\n", serial_rcv & 0xFF);
						}
					}
					break;

				default:
					break;
			}
		}
	}

	uart_unsubscribe();
	uart_reset(COM1_BASE);

	printf("Ended\n");
	return 0;
}
