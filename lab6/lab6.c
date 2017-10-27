#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/types.h>

#include "RTC.h"
#include "keyboard.h"
#include "timer.h"
#include "i8042.h"
#include "i8254.h"

int main(int argc, char **argv) {
    sef_startup();
    sys_enable_iop(SELF);

	rtc_date_t date;
	date.second = 0x00;
	date.minute = 0x53;
	date.hour = 0x17;
	date.dayOfMonth = 0x29;
	date.month = 0x12;
	date.year = 0x16;

	//rtc_set_date(date);
	//rtc_display_date();
	rtc_display_config();

    int32_t timer_irq = timer_subscribe_int();
    int32_t kbd_irq = kbd_subscribe_int();
    int32_t rtc_irq = rtc_subscribe();

    if(kbd_irq < 0 || rtc_irq < 0 || timer_irq < 0) {
    	return -1;
    }

    timer_irq = BIT(timer_irq);
    kbd_irq = BIT(kbd_irq);
    rtc_irq = BIT(rtc_irq);

    rtc_setalarm_s(15);

	//Variables for interrupt handling
	int ipc_status, dstatus;
	message msg;

	uint32_t kbd_code = 0;
	uint32_t counter = 0;
	uint32_t regC;

	//Interrupt handling
	while(kbd_code != ESC_BREAK) {
		if ((dstatus = driver_receive(ANY, &msg, &ipc_status)) != 0) { //Receives any messages
			printf("driver_receive failed with: %d", dstatus);
			continue;
		}

		//Check if notification
		if (is_ipc_notify(ipc_status)) {
			switch (_ENDPOINT_P(msg.m_source)) {
				case HARDWARE: //Hardware interrupt notification

					if(msg.NOTIFY_ARG & timer_irq) {
						if(counter % 60 == 0) {
							rtc_display_date();
						}
						counter++;
					}

					if(msg.NOTIFY_ARG & kbd_irq) {
						kbd_code = kbd_read();

						if(kbd_code == KBD_ERROR) {
							kbd_reset(ENABLED);
							timer_unsubscribe_int();
							rtc_unsubscribe();
							return -1;
						}
					}

					if (msg.NOTIFY_ARG & rtc_irq) {
						regC = rtc_read_register(RTC_STATUS_C);
						printf("regC: 0x%02X\n", regC);
					    if(rtc_setalarm_s(15) != 0) {
    						printf("error");
						}
					}

					break;
				default:

					break;
			}
		}
	}

	rtc_reset(TRUE);
	printf("Ended\n\n");

	kbd_reset(ENABLED);
	if(timer_unsubscribe_int() != 0) {
		return -1;
	}
	if(rtc_unsubscribe() != 0) {
		return -1;
	}

    return 0;
}
