#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/types.h>
#include <errno.h>
#include <limits.h>

#define SSIZE_MAX INT_MAX //MINIX defines ssize_t as typedef int ssize_t in types.h
#define _GETDELIM_GROWBY 128    /* amount to grow line buffer by */
#define _GETDELIM_MINLEN 4      /* minimum line buffer size */

//Source: https://gist.github.com/ingramj/1105106
ssize_t getdelim(char** lineptr, size_t* n, int delimiter, FILE* stream) {
	char *buf, *pos;
	int c;
	ssize_t bytes;

	if (lineptr == NULL || n == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (stream == NULL) {
		errno = EBADF;
		return -1;
	}

	/* resize (or allocate) the line buffer if necessary */
	buf = *lineptr;
	if (buf == NULL || *n < _GETDELIM_MINLEN) {
		buf = realloc(*lineptr, _GETDELIM_GROWBY);
		if (buf == NULL) {
			/* ENOMEM */
			return -1;
		}
		*n = _GETDELIM_GROWBY;
		*lineptr = buf;
	}

	/* read characters until delimiter is found, end of file is reached, or an
	   error occurs. */
	bytes = 0;
	pos = buf;
	while ((c = getc(stream)) != EOF) {
		if (bytes + 1 >= SSIZE_MAX) {
			errno = EOVERFLOW;
			return -1;
		}
		bytes++;
		if (bytes >= *n - 1) {
			buf = realloc(*lineptr, *n + _GETDELIM_GROWBY);
			if (buf == NULL) {
				/* ENOMEM */
				return -1;
			}
			*n += _GETDELIM_GROWBY;
			pos = buf + bytes - 1;
			*lineptr = buf;
		}

		if(c == delimiter) {
			break;
		}

		if(c != '\r' && c != '\n') {
			*pos++ = (char) c;
		}

	}

	if (ferror(stream) || (feof(stream) && (bytes == 0))) {
		/* EOF, or an error from getc(). */
		return -1;
	}

	*pos = '\0';
	return bytes;
}


unsigned long parse_ulong(char *str, int base) {
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


int16_t clamp_int16(int16_t value, int16_t min, int16_t max) {
	  const int16_t temp = value < min ? min : value;
	  return temp > max ? max : temp;
}
