#include <minix/syslib.h>
#include <minix/drivers.h>
#include "i8042.h"
#include "i8254.h"
#include "timer.h"
#include "mouse.h"
#include "keyboard.h"

//Displays the packets received from the mouse
//cnt - number of packets to print
int test_packet(unsigned short cnt) {

	if(cnt <= 0) {
		printf("mouse: test_packet: packet count should be higher than 0\n");
		return -1;
	}

	clearscreen();

	//Subscribe mouse interrupts
	int mouse_irq = mouse_subscribe_int();
	if(mouse_irq < 0) {
		return -1;
	}

	//Enable data reporting
	if(mouse_command(ENABLE_DATA) != 0) {
		mouse_reset(DISABLED);
		return -1;
	}

	//Handle mouse interrupts
	if(mouse_int_handler(BIT(mouse_irq), cnt) != 0) {
		mouse_reset(ENABLED);
		return -1; //All functions inside mouse_int_handler() already print errors
	}

	//Cleanup function, unsubscribes, clears output buffer
	mouse_reset(ENABLED);
	printf("Packet limit reached, exiting...\n\n");

	return 0;
}

//Displays the packets received from the mouse
//idle_time - seconds without packets to exit at
int test_async(unsigned short idle_time) {

	if(idle_time <= 0 || idle_time > 10) {
		printf("mouse: test_async: time value is restricted to a range from 1 to 10 seconds\n");
		return -1;
	}

	clearscreen();

	//Subscribe mouse interrupts
	int mouse_irq = mouse_subscribe_int();
	int timer_irq = timer_subscribe_int();

	if (mouse_irq < 0 || timer_irq < 0) {
		return -1;
	}

	//Enable data reporting
	if(mouse_command(ENABLE_DATA) != 0) {
		mouse_reset(DISABLED);
		timer_unsubscribe_int();
		return -1;
	}

	//Handle timer/mouse interrupts
	if(mouse_timed_handler(BIT(mouse_irq), BIT(timer_irq), idle_time) != 0) {
		mouse_reset(ENABLED);
		timer_unsubscribe_int();
		return -1; //All functions inside mouse_timed_handler() already print errors
	}

	//Cleanup function, unsubscribes, clears output buffer
	mouse_reset(ENABLED);
	if (timer_unsubscribe_int() != 0) {
		return -1;
	}
	printf("Idle time passed, exiting...\n\n");

	return 0;
}
	
//Displays mouse configuration in user friendly way
int test_config() {

	clearscreen();

	//Subscribe mouse interrupts
	int mouse_irq = mouse_subscribe_int();
	if(mouse_irq < 0) {
		return -1;
	}

	//Request config from mouse
	if(mouse_command(STATUS_REQUEST) != 0) {
		mouse_reset(DISABLED);
		return -1;
	}

	//Read config from mouse and print it
	mouse_config();

	//Cleanup function, unsubscribes, clears output buffer
	mouse_reset(DISABLED);

	return 0;
}
	
//Displays the packets received from the mouse but exists after a certain gesture has been detected
//length - length of the gesture needed to exit
int test_gesture(short length) {

	if(length == 0) {
		printf("mouse: test_gesture: 0 is not allowed for length of movement\n");
		return -1;
	}

	clearscreen();

	//Subscribe mouse interrupts
	int mouse_irq = mouse_subscribe_int();
	if(mouse_irq < 0) {
		return -1;
	}

	//Enable data reporting
	if(mouse_command(ENABLE_DATA) != 0) {
		mouse_reset(DISABLED);
		return -1;
	}

	//Handle interrupts and verify user gesture
	if(mouse_gesture_handler(BIT(mouse_irq), length) != 0) {
		mouse_reset(ENABLED);
		return -1; //All functions inside mouse_int_handler() already print errors
	}

	//Cleanup function, unsubscribes, clears output buffer
	mouse_reset(ENABLED);
	printf("Gesture detected, exiting...\n\n");

	return 0;
}

int test_remote(unsigned long period, unsigned short cnt) {

	clearscreen();

	//Subscribe mouse interrupts
	int mouse_irq = mouse_subscribe_int();
	if(mouse_irq < 0) {
		return -1;
	}

	int kbd_irq = kbd_subscribe_int();
	if(kbd_irq < 0) {
		mouse_reset(DISABLED);
		return -1;
	}

	//Enable remote mode
	if(mouse_command(REMOTE_MODE) != 0) {
		mouse_reset(DISABLED);
		kbd_reset(ENABLED);
		return -1;
	}

	//Read packets by polling and print them
	if(mouse_remote(period, cnt) != 0) {
		mouse_reset(DISABLED);
		kbd_reset(ENABLED);
		return -1;
	}

	//Cleanup function, unsubscribes, clears output buffer, restores stream mode
	mouse_reset(DISABLED);
	kbd_reset(ENABLED);
	printf("Packet limit reached, exiting...\n\n");

	return 0;
}
