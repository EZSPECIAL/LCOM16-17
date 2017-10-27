# Makefile for lab4

#Lists available options to the user (excluding make usage)
usage:	
	@echo " " >&2
	@echo "Makefile for timer (lab4)." >&2
	@echo "Root privileges are required." >&2
	@echo " " >&2
	@echo "Usage:" >&2
	@echo "	make                         # Default make target" >&2
	@echo "	make DEBUG=0|1 CLRSCR=0|1    # DEBUG adds extra output" >&2
	@echo "                                       CLRSCR defines if terminal should be" >&2
	@echo "                                       cleared for cleaner output" >&2
	@echo "        make clean                   # Remove .d .o files and compiled program" >&2
	@echo " " >&2

#Inform user of makefile usage
all:
	@echo "Use \"make usage\" for additional options" >&2

#Addition to clean so it removes .d files too
clean:
	rm *.d .depend

COMPILER_TYPE= gnu

CC= gcc

PROG= lab4
SRCS= lab4.c test4.c timer.c mouse.c

CCFLAGS= -Wall

#Toggle debug mode
.if "${DEBUG}" == "1"
CFLAGS+= -DDEBUG
.endif

#Toggle usage of clear screen command for cleaner output, uses ANSI escape codes
.if "${CLRSCR}" == "1"
CFLAGS+= -DCLRSCR
.endif

DPADD+= ${LIBDRIVER} ${LIBSYS}
LDADD+= -ldriver -lsys

LDFLAGS+= -L .

MAN=

.include <bsd.gcc.mk>
.include <bsd.prog.mk>