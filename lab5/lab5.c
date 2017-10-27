#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <minix/syslib.h>
#include "test5.h"
#include "pixmap.h" //Sprites

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
			"       service run %s -args \"init <mode - int, delay - 1-10>\"\n"
			"       service run %s -args \"square <x, y, size, color>\"\n"
			"       service run %s -args \"line <xi, yi, xi, yf, color>\"\n"
			"       service run %s -args \"xpm <xi, yi, xpm>\"\n"
			"       service run %s -args \"move <xi, yi, xpm, hor, delta, time>\"\n"
			"       service run %s -args \"controller\"\n",
			argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
}


//Process arguments passed through MINIX terminal
static int proc_args(int argc, char **argv) {
	unsigned long mode, time, x, y, xf, yf, size, color, hor, delta;

	//test_init(mode, delay)
	if (strncmp(argv[1], "init", strlen("init")) == 0) {
		if (argc != 4) {
			printf("vga: wrong number of arguments for test_init()\n");
			return 1;
		}

		//Parse mode parameter
		mode = parse_ulong(argv[2], 16);
		if (mode == ULONG_MAX) {
			printf("vga: mode value out of range or not a number\n");
			return 1;
		}

		//Parse delay parameter
		time = parse_ulong(argv[3], 10);
		if (time == ULONG_MAX) {
			printf("vga: delay value out of range or not a number\n");
			return 1;
		}

		printf("vga::test_init(0x%02X, %lu)\n\n", mode, time);
		return (uintptr_t)test_init(mode, time);
	}

	//test_square(x, y, size, color)
	else if (strncmp(argv[1], "square", strlen("square")) == 0) {
		if (argc != 6) {
			printf("vga: wrong number of arguments for test_square()\n");
			return 1;
		}

		//Parse x parameter
		x = parse_ulong(argv[2], 10);
		if (x == ULONG_MAX) {
			printf("vga: x value out of range or not a number\n");
			return 1;
		}

		//Parse y parameter
		y = parse_ulong(argv[3], 10);
		if (y == ULONG_MAX) {
			printf("vga: y value out of range or not a number\n");
			return 1;
		}

		//Parse size parameter
		size = parse_ulong(argv[4], 10);
		if (size == ULONG_MAX) {
			printf("vga: size value out of range or not a number\n");
			return 1;
		}

		//Parse color parameter
		color = parse_ulong(argv[5], 16);
		if (color == ULONG_MAX) {
			printf("vga: color value out of range or not a number\n");
			return 1;
		}

		printf("vga::test_square(%lu, %lu, %lu, 0x%02X)\n\n", x, y, size, color);
		return test_square(x, y, size, color);
	}

	//test_line(xi, yi, xf, yf, color)
	else if (strncmp(argv[1], "line", strlen("line")) == 0) {
		if (argc != 7) {
			printf("vga: wrong number of arguments for test_line()\n");
			return 1;
		}

		//Parse xi parameter
		x = parse_ulong(argv[2], 10);
		if (x == ULONG_MAX) {
			printf("vga: xi value out of range or not a number\n");
			return 1;
		}

		//Parse yi parameter
		y = parse_ulong(argv[3], 10);
		if (y == ULONG_MAX) {
			printf("vga: xf value out of range or not a number\n");
			return 1;
		}

		//Parse xf parameter
		xf = parse_ulong(argv[4], 10);
		if (xf == ULONG_MAX) {
			printf("vga: xf value out of range or not a number\n");
			return 1;
		}

		//Parse yf parameter
		yf = parse_ulong(argv[5], 10);
		if (yf == ULONG_MAX) {
			printf("vga: yf value out of range or not a number\n");
			return 1;
		}

		//Parse color parameter
		color = parse_ulong(argv[6], 16);
		if (color == ULONG_MAX) {
			printf("vga: color value out of range or not a number\n");
			return 1;
		}

		printf("vga::test_line(%lu, %lu, %lu, %lu, 0x%02X)\n\n", x, y, xf, yf, color);
		return test_line(x, y, xf, yf, color);
	}

	//test_xpm(xi, yi, xpm)
	else if (strncmp(argv[1], "xpm", strlen("xpm")) == 0) {
		if (argc != 5) {
			printf("vga: wrong number of arguments for test_xpm()\n");
			return 1;
		}

		//Parse xi parameter
		x = parse_ulong(argv[2], 10);
		if (x == ULONG_MAX) {
			printf("vga: xi value out of range or not a number\n");
			return 1;
		}

		//Parse yi parameter
		y = parse_ulong(argv[3], 10);
		if (y == ULONG_MAX) {
			printf("vga: yi value out of range or not a number\n");
			return 1;
		}

		//Parse xpm parameter
		if (strncmp(argv[4], "pic1", strlen("pic1")) == 0) {
			printf("vga::test_xpm(%lu, %lu, %s)\n\n", x, y, argv[4]);
			return test_xpm(x, y, pic1);
		}
		else if (strncmp(argv[4], "pic2", strlen("pic2")) == 0) {
			printf("vga::test_xpm(%lu, %lu, %s)\n\n", x, y, argv[4]);
			return test_xpm(x, y, pic2);
		}
		else if (strncmp(argv[4], "pic3", strlen("pic3")) == 0) {
			printf("vga::test_xpm(%lu, %lu, %s)\n\n", x, y, argv[4]);
			return test_xpm(x, y, pic3);
		}
		else if (strncmp(argv[4], "cross", strlen("cross")) == 0) {
			printf("vga::test_xpm(%lu, %lu, %s)\n\n", x, y, argv[4]);
			return test_xpm(x, y, cross);
		}
		else if (strncmp(argv[4], "penguin", strlen("penguin")) == 0) {
			printf("vga::test_xpm(%lu, %lu, %s)\n\n", x, y, argv[4]);
			return test_xpm(x, y, penguin);
		}
	}

	//test_move(xi, yi, xpm, hor, delta, time)
	else if (strncmp(argv[1], "move", strlen("move")) == 0) {
		if (argc != 8) {
			printf("vga: wrong number of arguments for test_move()\n");
			return 1;
		}

		//Parse xi parameter
		x = parse_ulong(argv[2], 10);
		if (x == ULONG_MAX) {
			printf("vga: xi value out of range or not a number\n");
			return 1;
		}

		//Parse yi parameter
		y = parse_ulong(argv[3], 10);
		if (y == ULONG_MAX) {
			printf("vga: yi value out of range or not a number\n");
			return 1;
		}

		//Parse hor parameter
		hor = parse_ulong(argv[5], 10);
		if (hor == ULONG_MAX) {
			printf("vga: hor value out of range or not a number\n");
			return 1;
		}

		//Parse delta parameter
		delta = parse_long(argv[6], 10);
		if (delta == LONG_MAX) {
			printf("vga: delta value out of range or not a number\n");
			return 1;
		}

		//Parse time parameter
		time = parse_ulong(argv[7], 10);
		if (time == ULONG_MAX) {
			printf("vga: time value out of range or not a number\n");
			return 1;
		}

		//Parse xpm parameter
		if (strncmp(argv[4], "pic1", strlen("pic1")) == 0) {
			printf("vga::test_move(%lu, %lu, %s, %lu, %lu, %lu)\n\n", x, y, argv[4], hor, delta, time);
			return test_move(x, y, pic1, hor, delta, time);
		}
		else if (strncmp(argv[4], "pic2", strlen("pic2")) == 0) {
			printf("vga::test_move(%lu, %lu, %s, %lu, %lu, %lu)\n\n", x, y, argv[4], hor, delta, time);
			return test_move(x, y, pic2, hor, delta, time);
		}
		else if (strncmp(argv[4], "pic3", strlen("pic3")) == 0) {
			printf("vga::test_move(%lu, %lu, %s, %lu, %lu, %lu)\n\n", x, y, argv[4], hor, delta, time);
			return test_move(x, y, pic3, hor, delta, time);
		}
		else if (strncmp(argv[4], "cross", strlen("cross")) == 0) {
			printf("vga::test_move(%lu, %lu, %s, %lu, %lu, %lu)\n\n", x, y, argv[4], hor, delta, time);
			return test_move(x, y, cross, hor, delta, time);
		}
		else if (strncmp(argv[4], "penguin", strlen("penguin")) == 0) {
			printf("vga::test_move(%lu, %lu, %s, %lu, %lu, %lu)\n\n", x, y, argv[4], hor, delta, time);
			return test_move(x, y, penguin, hor, delta, time);
		}
	}

	//test_controller
	else if (strncmp(argv[1], "controller", strlen("controller")) == 0) {
		if (argc != 2) {
			printf("vga: wrong number of arguments for test_controller()\n");
			return 1;
		}

		printf("vga::test_controller()\n\n");
		return test_controller();
	}

	else {
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
#if defined(DEBUG) && DEBUG == 1
		char *errorstr = strerror(errsv);
		printf("vga: parse_ulong: error %d - %s\n", errsv, errorstr);
#endif
		return ULONG_MAX;
	}

	if (endptr == str) {
#if defined(DEBUG) && DEBUG == 1
		printf("vga: parse_ulong: no digits were found in \"%s\"\n", str);
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
