#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <minix/syslib.h>
#include "test4.h"
#include "i8042.h"

static int proc_args(int argc, char **argv);
static unsigned long parse_ulong(char *str, int base);
static void print_usage(char **argv);
static long parse_long(char *str, int base);

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
static void print_usage(char **argv) {
	printf("Usage:\n"
			"       service run %s -args \"packet <cnt - >0>\"\n"
			"       service run %s -args \"async <time - 1-10>\"\n"
			"       service run %s -args \"config\"\n"
			"       service run %s -args \"gesture <length - !=0>\"\n"
			"       service run %s -args \"remote <period - ms> <cnt - >0>\"\n",
			argv[0], argv[0], argv[0], argv[0], argv[0]);
}


//Process arguments passed from user by MINIX terminal
static int proc_args(int argc, char **argv) {
	unsigned long cnt, time, length, period;

	//test_packet(cnt)
	if (strncmp(argv[1], "packet", strlen("packet")) == 0) {
		if (argc != 3) {
			printf("mouse: wrong number of arguments for test_packet()\n");
			return 1;
		}

		//Parse cnt parameter
		cnt = parse_ulong(argv[2], 10);
		if (cnt == ULONG_MAX) {
			printf("mouse: cnt value out of range or not a number\n");
			return 1;
		}

		printf("mouse::test_packet(%lu)\n\n", cnt);
		return test_packet(cnt);
	}

	//test_async(idle_time);
	else if (strncmp(argv[1], "async", strlen("async")) == 0) {
		if (argc != 3) {
			printf("mouse: wrong number of arguments for test_async()\n");
			return 1;
		}

		//Parse time parameter
		time = parse_ulong(argv[2], 10);
		if (time == ULONG_MAX) {
			printf("mouse: time value out of range or not a number\n");
			return 1;
		}

		printf("mouse::test_async(%lu)\n\n", time);
		return test_async(time);
	}

	//test_config()
	else if (strncmp(argv[1], "config", strlen("config")) == 0) {
		if (argc != 2) {
			printf("mouse: wrong number of arguments for test_config()\n");
			return 1;
		}

		printf("mouse::test_config()\n\n");
		return test_config();
	}

	//test_gesture(length)
	else if (strncmp(argv[1], "gesture", strlen("gesture")) == 0) {
		if (argc != 3) {
			printf("mouse: wrong number of arguments for test_gesture()\n");
			return 1;
		}

		//Parse length parameter
		length = parse_long(argv[2], 10);
		if (length == LONG_MAX) {
			printf("mouse: length value out of range or not a number\n");
			return 1;
		}

		printf("mouse::test_gesture(%ld)\n\n", length);
		return test_gesture(length);
	}

	//test_remote(period, cnt)
	else if (strncmp(argv[1], "remote", strlen("remote")) == 0) {
		if (argc != 4) {
			printf("mouse: wrong number of arguments for test_remote()\n");
			return 1;
		}

		//Parse period parameter
		period = parse_long(argv[2], 10);
		if (period == LONG_MAX) {
			printf("mouse: period value out of range or not a number\n");
			return 1;
		}

		//Parse cnt parameter
		cnt = parse_ulong(argv[3], 10);
		if (cnt == ULONG_MAX) {
			printf("mouse: cnt value out of range or not a number\n");
			return 1;
		}

		printf("mouse::test_remote(%ld, %ld)\n\n", period, cnt);
		return test_remote(period, cnt);
	} else {
		printf("mouse: %s - not a valid function!\n", argv[1]);
		return 1;
	}
}

static unsigned long parse_ulong(char *str, int base) {
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
		printf("mouse: parse_ulong: error %d - %s\n", errsv, errorstr);
#endif
		return ULONG_MAX;
	}

	if (endptr == str) {
#if defined(DEBUG) && DEBUG == 1
		printf("mouse: parse_ulong: no digits were found in \"%s\"\n", str);
#endif
		return ULONG_MAX;
	}

	//Successful conversion
	return val;
}

static long parse_long(char *str, int base) {
	char *endptr;
	unsigned long val;
	int errsv; //Preserve errno

	//Convert string to unsigned long
	errno = 0; //Clear errno
	val = strtol(str, &endptr, base);
    errsv = errno;

	//Check for conversion errors
    if (errsv != 0) {
#if defined(DEBUG) && DEBUG == 1
		char *errorstr = strerror(errsv);
		printf("mouse: parse_long: error %d - %s\n", errsv, errorstr);
#endif
		return LONG_MAX;
	}

	if (endptr == str) {
#if defined(DEBUG) && DEBUG == 1
		printf("mouse: parse_long: no digits were found in \"%s\"\n", str);
#endif
		return LONG_MAX;
	}

	//Successful conversion
	return val;
}
