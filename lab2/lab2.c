#include "timer.h"
#include "i8254.h"
#include <limits.h>
#include <string.h>
#include <errno.h>

static int proc_args(int argc, char **argv);
static unsigned long parse_ulong(char *str, int base);
static void print_usage(char **argv);

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
			"       service run %s -args \"config <timer - integer>\"\n"
			"       service run %s -args \"square <frequency - integer>\"\n"
			"       service run %s -args \"int <time - integer>\"\n",
			argv[0], argv[0], argv[0]);
}


//Process arguments passed from user by MINIX terminal
static int proc_args(int argc, char **argv)
{
	unsigned long timer, freq, time;

	//timer_test_config(timer)
	if (strncmp(argv[1], "config", strlen("config")) == 0) {
		if (argc != 3) {
			printf("timer: wrong number of arguments for timer_test_config()\n");
			return 1;
		}

		//Parse timer parameter
		timer = parse_ulong(argv[2], 10);
		if (timer == ULONG_MAX) {
			printf("timer: timer value out of range or not a number, please enter [0-2]\n");
			return 1;
		}

		printf("timer::timer_test_config(%lu)\n\n", timer);
		return timer_test_config(timer);
	}

	//timer_test_square(freq)
	else if (strncmp(argv[1], "square", strlen("square")) == 0) {
		if (argc != 3) {
			printf("timer: wrong number of arguments for timer_test_square()\n");
			return 1;
		}

		//Parse freq parameter
		freq = parse_ulong(argv[2], 10);
		if (freq == ULONG_MAX) {
			printf("timer: freq value out of range or not a number\n");
			return 1;
		}

		printf("timer::timer_test_square(%lu)\n\n", freq);
		return timer_test_square(freq);
	}

	//timer_test_int(time)
	else if (strncmp(argv[1], "int", strlen("int")) == 0) {
		if (argc != 3) {
			printf("timer: wrong number of arguments for timer_test_int()\n");
			return 1;
		}

		//Parse time parameter
		time = parse_ulong(argv[2], 10);
		if (time == ULONG_MAX) {
			printf("timer: time value out of range or not a number\n");
			return 1;
		}

		printf("timer::timer_test_int(%lu)\n\n", time);
		return timer_test_int(time);
	} else {
		printf("timer: %s - not a valid function!\n", argv[1]);
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
		printf("timer: parse_ulong: error %d - %s\n", errsv, errorstr);
#endif
		return ULONG_MAX;
	}

	if (endptr == str) {
#if defined(DEBUG) && DEBUG == 1
		printf("timer: parse_ulong: no digits were found in \"%s\"\n", str);
#endif
		return ULONG_MAX;
	}

	//Successful conversion
	return val;
}
