#include <minix/syslib.h>
#include <minix/drivers.h>
#include "i8254.h"

static int timer_hook; //Timer 0 hook id

//Subscribe timer interrupts
int timer_subscribe_int(void ) {
	timer_hook = TIMER_HOOK_BIT;
    if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &timer_hook) == OK) {
    	if (sys_irqenable(&timer_hook) == OK) {
#if defined(DEBUG) && DEBUG == 1
    		printf("Hook id returned: 0x%02X\n\n", timer_hook);
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
    if(sys_irqdisable(&timer_hook) == OK) {
    	if(sys_irqrmpolicy(&timer_hook) == OK) {
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


//Uses timer to sleep for "time" seconds
int timer_sleep(int time) {

	//Subscribe timer interrupts
	int hook = timer_subscribe_int();
	int counter = 0;

	if (hook >= 0) { //Check if valid hook id

		//Variables for interrupt handling
		int ipc_status, dstatus;
		message msg;
		int irq_set = BIT(hook); //bitmask for timer interrupts
		float freq = 60; //Frequency used by the timer interrupt

		//Interrupt handling
		while(counter < time * freq) { //counter is updated at freq/s so we multiply freq * time to get counter value at time (seconds)
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

		//Reset parameters
		counter = 0;

		//Unsubscribe timer interrupts
		if (timer_unsubscribe_int() == 0) {
			return 0;
		} else return -1; //Failed to unsubscribe timer interrupts
	} else return -1; //Failed to subscribe timer interrupts
}
