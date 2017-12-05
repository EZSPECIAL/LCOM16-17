#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <minix/syslib.h>
#include "test5.h"

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
static void print_usage(char **argv) {
	printf("Usage:\n"
			"       service run %s -args \"png <24-bit mode> <png path>\"\n"
			"       example: \"png 0x115 /home/stbimage/test.png\"\n",
			argv[0]);
}

//Process arguments passed through MINIX terminal
static int proc_args(int argc, char **argv) {

	unsigned long mode;

	//test_png(mode, path)
	if (strncmp(argv[1], "png", strlen("png")) == 0) {
		if (argc != 4) {
			printf("vga: wrong number of arguments for test_png()\n");
			return 1;
		}

		//Parse mode parameter
		mode = parse_ulong(argv[2], 16);
		if (mode == ULONG_MAX) {
			printf("vga: mode value out of range or not a number\n");
			return 1;
		}

		printf("vga::test_png(0x%02X, %s)\n\n", mode, argv[3]);
		return test_png(mode, argv[3]);
	} else {
		printf("vga: %s - not a valid function!\n", argv[1]);
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
		return ULONG_MAX;
	}

	if (endptr == str) {
		return ULONG_MAX;
	}

	//Successful conversion
	return val;
}
