#include <minix/syslib.h>
#include <minix/drivers.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include "LoLCOM.h"
#include "logic.h"
#include "video_gr.h"
#include "i8042.h"
#include "keyboard.h"
#include "timer.h"
#include "video.h"
#include "RTC.h"
#include "mouse.h"
#include "helper.h"
#include "UART.h"
#include "speaker.h"

static int proc_args(int argc, char **argv);
static void print_usage(char **argv);
int8_t lolcom_player1();
int8_t lolcom_player2();

int main(int argc, char **argv) {
	sef_startup();
	sys_enable_iop(SELF);
	srand(time(NULL));

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
			"       service run %s -args \"player1\"\n"
			"       service run %s -args \"player2\"\n"
			"       service run %s -args \"speakerPWM <bitrate, filename>\"\n"
			"       service run %s -args \"speaker1bit <filename>\"\n",
			argv[0], argv[0], argv[0], argv[0]);
}


//Process arguments passed from user by MINIX terminal
static int proc_args(int argc, char **argv)
{
	uint32_t use_double_buffer, freq;

	//LoLCOM_player1()
	if (strncmp(argv[1], "player1", strlen("player1")) == 0) {
		if (argc != 2) {
			printf("LoLCOM: wrong number of arguments for LoLCOM_player1()\n");
			return 1;
		}

		return lolcom_player1();
	}

	//LoLCOM_player2()
	else if (strncmp(argv[1], "player2", strlen("player2")) == 0) {
		if (argc != 2) {
			printf("LoLCOM: wrong number of arguments for LoLCOM_player2()\n");
			return 1;
		}

		return lolcom_player2();

	//LoLCOM_speakerPWM()
	} else if(strncmp(argv[1], "speakerPWM", strlen("speakerPWM")) == 0) {
		if (argc != 4) {
			printf("LoLCOM: wrong number of arguments for LoLCOM_speakerPWM()()\n");
			return 1;
		}

		//Parse freq parameter
		freq = parse_ulong(argv[2], 10);
		if (freq == ULONG_MAX || freq < 8000 || freq > 44100) {
			printf("LoLCOM: freq value out of range (8000 - 44100)\n");
			return 1;
		}

		return speaker_PWM(freq, argv[3]);

	//LoLCOM_speaker1bit()
	} else if(strncmp(argv[1], "speaker1bit", strlen("speaker1bit")) == 0) {
		if (argc != 3) {
			printf("LoLCOM: wrong number of arguments for LoLCOM_speaker1bit()\n");
			return 1;
		}

		return speaker_square(argv[2]);
	} else {
		printf("LoLCOM: %s - not a valid function!\n", argv[1]);
		return 1;
	}
}


int8_t lolcom_player1() {

	if(logic_menu_init() != 0) {
		return -1;
	}

	UART_config_t config;

	uart_set_conf(COM1_BASE, receive8N1_9600);

	vg_init_values(VMODE);
	vg_init(VMODE);

	int32_t kbd_irq = kbd_subscribe_int();
	int32_t timer_irq = timer_subscribe_int();
	int32_t rtc_irq = rtc_subscribe();
	int32_t mouse_irq = mouse_subscribe_int();
	int32_t com1_irq = uart_subscribe();

	if (kbd_irq < 0 || timer_irq < 0 || rtc_irq < 0 || mouse_irq < 0 || com1_irq < 0) {
		printf("LoLCOM: failed to subscribe one or more device interrupts\n");
		return -1;
	}

	//Try to enable 4th mouse packet
	uint8_t id = mouse_magic_sequence();

	if(id != MOUSE_SCROLL_EX) {
		mouse_command(MOUSE_DEFAULTS);
		id = 0;
	}

	//Enable data reporting for mouse
	if(mouse_command(ENABLE_DATA) != 0) {
		mouse_reset(DISABLED);
		return -1;
	}

	//Variables for interrupt handling
	kbd_irq = BIT(kbd_irq);
	timer_irq = BIT(timer_irq);
	rtc_irq = BIT(rtc_irq);
	mouse_irq = BIT(mouse_irq);
	com1_irq = BIT(com1_irq);

	int ipc_status, dstatus;
	message msg;

	uint32_t kbd_code = 0;
	uint8_t pnumber = 0;
	uint8_t sync = 0;
	uint32_t mouse_data;
	uint32_t serial_rcv = 0;
	uint8_t end = FALSE;

	//Game loop continues until user presses ESC
	while(end == FALSE) {
		if ((dstatus = driver_receive(ANY, &msg, &ipc_status)) != 0) { //Receives any messages
			printf("driver_receive failed with: %d", dstatus);
			continue;
		}

		//Check if notification
		if (is_ipc_notify(ipc_status)) {
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE: //Hardware interrupt notification

				//Timer interrupt
				if(msg.NOTIFY_ARG & timer_irq) {

					//Game loop
					logic_handler(0, NULL, NULL, 0, TIMER_INT);
					end = logic_check_end();
				}

				//Keyboard interrupts
				if(msg.NOTIFY_ARG & kbd_irq) {

					//Read output port for scancode
					kbd_code = kbd_read();

					//Check scancode read success
					if (kbd_code == KBD_ERROR) {
						mouse_reset(ENABLED);
						kbd_reset(ENABLED);
						rtc_reset(TRUE);
						rtc_unsubscribe();
						timer_unsubscribe_int();
						uart_reset(COM1_BASE);
						uart_unsubscribe();
						vg_exit();
						vg_free();
						printf("LoLCOM: keyboard: error reading from output buffer\n");
						return -1;
					}
					logic_handler(kbd_code, NULL, NULL, GAME, KBD_INT);
				}

				//RTC Alarm interrupts
				if(msg.NOTIFY_ARG & rtc_irq) {
					logic_handler(0, NULL, NULL, 0, RTC_INT);
				}

				//Mouse interrupt
				if(msg.NOTIFY_ARG & mouse_irq) {

					//Read output port for scancode
					if(sys_inb(KBD_OUT_BUF, &mouse_data) != OK) {
						mouse_reset(ENABLED);
						kbd_reset(ENABLED);
						rtc_reset(TRUE);
						rtc_unsubscribe();
						timer_unsubscribe_int();
						uart_reset(COM1_BASE);
						uart_unsubscribe();
						vg_exit();
						vg_free();
						printf("LoLCOM: mouse: error reading from output buffer\n");
						return -1;
					}
					logic_handler(mouse_data, &pnumber, &sync, id, MOUSE_INT);
				}

				//COM1 Interrupt
				if (msg.NOTIFY_ARG & com1_irq) {

					serial_rcv = uart_receive();

					if(serial_rcv == UART_ERROR) {
						mouse_reset(ENABLED);
						kbd_reset(ENABLED);
						rtc_reset(TRUE);
						rtc_unsubscribe();
						timer_unsubscribe_int();
						uart_reset(COM1_BASE);
						uart_unsubscribe();
						vg_exit();
						vg_free();
						return -1;
					} else if(serial_rcv == RCV_ERROR) {
						printf("COM1: error in transmission\n");
					} else {
						logic_handler(serial_rcv, NULL, NULL, 0, SERIAL_INT);
					}
				}
				break;

			default: //No other notifications expected, do nothing
				break;
			}
		}
	}

	mouse_reset(ENABLED);
	kbd_reset(ENABLED);
	rtc_reset(TRUE);
	rtc_unsubscribe();
	timer_unsubscribe_int();
	uart_reset(COM1_BASE);
	uart_unsubscribe();

	//Frees all allocated memory for the game data and also frees VRAM
	vg_exit();
	vg_free();

	return 0;
}


int8_t lolcom_player2() {

	UART_config_t config;

	uart_set_conf(COM1_BASE, transmit8N1_9600);

	if(logic_serial_init() != 0) {
		return -1;
	}

	vg_init_values(VMODE);
	vg_init(VMODE);

	int32_t timer_irq = timer_subscribe_int();
	int32_t kbd_irq = kbd_subscribe_int();

	if(kbd_irq < 0 || timer_irq < 0) {
		printf("LoLCOM: failed to subscribe one or more device interrupts\n");
		return -1;
	}

	timer_irq = BIT(timer_irq);
	kbd_irq = BIT(kbd_irq);

	int ipc_status, dstatus;
	message msg;
	uint32_t kbd_code;
	uint32_t counter = 0;

	//Handle interrupts until user depresses ESC key
	while(kbd_code != BREAK(ESC_MAKE)) {
		if ((dstatus = driver_receive(ANY, &msg, &ipc_status)) != 0) { //Receives any messages
			printf("driver_receive failed with: %d", dstatus);
			continue;
		}

		//Check if notification
		if (is_ipc_notify(ipc_status)) {
			switch (_ENDPOINT_P(msg.m_source)) {
				case HARDWARE: //Hardware interrupt notification

					//Timer interrupt
					if(msg.NOTIFY_ARG & timer_irq) {
						if(counter % 60 == 0) {
							logic_serial_tick();
							counter = 0;
						}
						logic_display_serial();
						counter++;
					}

					//Keyboard interrupt
					if (msg.NOTIFY_ARG & kbd_irq) {

						//Read output port for scancode
						kbd_code = kbd_read();

						//Check scancode read success
						if (kbd_code == KBD_ERROR) {
							kbd_reset(ENABLED);
							timer_unsubscribe_int();
							uart_reset(COM1_BASE);
							vg_exit();
							logic_serial_free();
							return -1;
						}

						logic_kbd_input(kbd_code, PLAYER2);
					}
					break;

				default:
					break;
			}
		}
	}

	kbd_reset(ENABLED);
	timer_unsubscribe_int();
	uart_reset(COM1_BASE);
	vg_exit();
	logic_serial_free();

	return 0;
}
