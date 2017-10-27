#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/driver.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include "i8042.h"
#include "i8254.h"
#include "timer.h"
#include "keyboard.h"

//Displays scancodes received from the keyboard, exits on ESC key break code
//ass - defines whether to use C or asm handler
int kbd_test_scan(unsigned short ass) {

	clearscreen();

	//Subscribe keyboard interrupts
	int kbd_irq = kbd_subscribe_int();
	if (kbd_irq < 0) {
		return -1;
	}

	if(ass) {
		printf("Using assembly handler\n\n");
	} else printf("Using C handler\n\n");

	if (kbd_int_handler(BIT(kbd_irq), ass) != 0) {
		return -1; //All functions inside kbd_int_handler() already print errors
	}

	//Cleanup function, unsubscribes, clears output buffer
	kbd_reset(ENABLED);
	printf("Received \"ESC\" key break code, exiting...\n\n");

	return 0;
}

//Toggles LED indicators, one per second, according to the arguments
int kbd_test_leds(unsigned short n, unsigned short *leds) {

	clearscreen();

	int i;
	short led_status = 0;

	//User sanity check
	if (n < 1 || n > 30) {
		printf("keyboard: kbd_test_leds: size of array is restricted to a range from 1 to 30 elements\n");
		return -1;
	}

	//Check array values
	for (i = 0; i < n; i++) {
		if(leds[i] < 0 || leds[i] > 2) {
			printf("keyboard: kbd_test_leds: array value out of range or wrong size\n");
			return -1;
		}
	}

	//Subscribe keyboard interrupts so MINIX doesn't steal responses to commands
	if (kbd_subscribe_int() < 0) {
		return -1; //kbd_subscribe_int() prints error message already
	}

	//Disables scancodes so they don't interfere with responses
	if(kbd_command(KBD_DISABLE, KBD_IN_BUF) != 0) {
		kbd_reset(DISABLED);
		return -1;
	}

	//Iterate through leds
	for(i = 0; i < n; i++) {

		//Uses timer to sleep 1 second
		timer_sleep(1);

		//Toggle bit on position leds[i]
		led_status ^= BIT(leds[i]);

		//Write 0xED to keyboard on port 0x60
		if(kbd_command(KBD_LED, KBD_IN_BUF) != 0) {
			kbd_reset(DISABLED);
			return -1;
		}

		//Write led_status to keyboard on port 0x60
		if(kbd_command(led_status, KBD_IN_BUF) != 0) {
			kbd_reset(DISABLED);
			return -1;
		}

		printf("SCROLL: %d NUM: %d CAPS: %d\n", led_status & BIT(0), (led_status & BIT(1)) >> 1, (led_status & BIT(2)) >> 2);
	}

	//Re-enable scancodes
	if(kbd_command(KBD_ENABLE, KBD_IN_BUF) != 0) {
		kbd_reset(ENABLED);
		return -1;
	}

	//Cleanup function, unsubscribes, clears output buffer
	kbd_reset(ENABLED);
	printf("Finished sequence\n\n");

	return 0;
}


//Displays scancodes received from the keyboard, exits on ESC key break code or after "n" seconds
int kbd_test_timed_scan(unsigned short n) {

	clearscreen();

	//Limit time to reasonable values
	if (n <= 0 || n > 10) {
		printf("keyboard: kbd_test_timed_scan: time value is restricted to a range from 1 to 10 seconds\n");
		return -1;
	}

	//Subscribe timer/keyboard interrupts
	int timer_hook = timer_subscribe_int();
	int kbd_hook = kbd_subscribe_int();

	if (kbd_timed_handler(BIT(kbd_hook), BIT(timer_hook), n) != 0) {
		return -1; //All functions inside kbd_timed_handler() already print errors
	}

	//Cleanup function, unsubscribes, clears output buffer
	kbd_reset(ENABLED);
	if (timer_unsubscribe_int() != 0) {
		return -1;
	}
	printf("Received \"ESC\" key break code or timed out, exiting...\n\n");

	return 0;
}

int kbd_test_poll() {

	if(kbd_poll() != 0) {
		printf("keyboard: kbd_test_poll: an error occurred, resetting KBC.\n");
		kbd_reset(ENABLED);
	}

	//Cleanup function, unsubscribes, clears output buffer
	kbd_reset(ENABLED);
	printf("Received \"ESC\" key break code, exiting...\n\n");
	return 0;
}
