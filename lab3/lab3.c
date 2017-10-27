#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <minix/syslib.h>
#include "test3.h"
#include "i8042.h"


static int proc_args(int argc, char **argv);
static unsigned long parse_ulong(char *str, int base);
static void print_usage(char **argv);

int main(int argc, char **argv) {
	sef_startup();
    sys_enable_iop(SELF);

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
			"       service run %s -args \"scan <asm - 0 for C, >0 for asm>\"\n"
			"       service run %s -args \"leds <size - integer, *leds - array>\"\n"
			"       service run %s -args \"timed <time - integer>\"\n"
			"       service run %s -args \"poll\"\n",
			argv[0], argv[0], argv[0], argv[0]);
}


//Process arguments passed from user by MINIX terminal
static int proc_args(int argc, char **argv)
{
	unsigned long ass;
	unsigned long n = 0;

	//kbd_test_scan(ass);
	if (strncmp(argv[1], "scan", strlen("scan")) == 0) {
		if (argc != 3) {
			printf("keyboard: wrong number of arguments for kbd_test_scan()\n");
			return 1;
		}

		//Parse ass parameter
		ass = parse_ulong(argv[2], 10);
		if (ass == ULONG_MAX) {
			printf("keyboard: ass value out of range or not a number\n");
			return 1;
		}

		printf("keyboard::keyboard_test_scan(%lu)\n\n", ass);
		return kbd_test_scan(ass);
	}

	//kbd_test_leds(n, *leds);
	else if (strncmp(argv[1], "leds", strlen("leds")) == 0) {
		if (argc != 4) {
			printf("keyboard: wrong number of arguments for kbd_test_leds()\n");
			return 1;
		}

		//Parse n parameter
		n = parse_ulong(argv[2], 10);
		if (n == ULONG_MAX) {
			printf("timer: freq value out of range or not a number\n");
			return 1;
		}

		//Parse leds parameter
		unsigned short leds[n];
		char number[1];
		int i;
		for(i = 0; i < n; i++)
		{
			number[0] = argv[3][i]; //Get array digit by digit
			leds[i] = parse_ulong(number, 10);
			if (leds[i] == MAX_UINT16) { //Conversion failed, either not a digit or null termination '\0'
				break; //This effectively stops the user from being able to provide an array out of range, "leds 3 1" would stop at '\0' and then fail on the input check
			}
		}

		printf("keyboard::kbd_test_leds(%lu, %.*s)\n\n", n, n, argv[3]); //Print straight from argv since leds is not a char*
		return kbd_test_leds(n, leds);
	}

	//kbd_test_timed_scan(n);
	else if (strncmp(argv[1], "timed", strlen("timed")) == 0) {
		if (argc != 3) {
			printf("keyboard: wrong number of arguments for kbd_test_timed_scan()\n");
			return 1;
		}

		//Parse n parameter
		n = parse_ulong(argv[2], 10);
		if (n == ULONG_MAX) {
			printf("keyboard: n value out of range or not a number\n");
			return 1;
		}

		printf("keyboard::kbd_test_timed_scan(%lu)\n\n", n);
		return kbd_test_timed_scan(n);
	} else if (strncmp(argv[1], "poll", strlen("poll")) == 0) {
		if (argc != 2) {
			printf("keyboard: wrong number of arguments for kbd_test_poll()\n");
			return 1;
		}

		printf("keyboard::kbd_test_poll\n\n", n);
		return kbd_test_poll();
	} else {
		printf("keyboard: %s - not a valid function!\n", argv[1]);
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
#if defined(DEBUG) && DEBUG == 1
		char *errorstr = strerror(errsv);
		printf("keyboard: parse_ulong: error %d - %s\n", errsv, errorstr);
#endif
		return ULONG_MAX;
	}

	if (endptr == str) {
#if defined(DEBUG) && DEBUG == 1
		printf("keyboard: parse_ulong: no digits were found in \"%s\"\n", str);
#endif
		return ULONG_MAX;
	}

	//Successful conversion
	return val;
}
