# Makefile for proj

#Lists available options to the user (excluding make usage)
usage:	
	@echo " " >&2
	@echo "Makefile for Legend of LCOM." >&2
	@echo "Root privileges are required." >&2
	@echo " " >&2
	@echo "Usage:" >&2
	@echo "	make                         # Default make target" >&2
	@echo "	make DEBUG=0|1               # DEBUG adds extra output" >&2
	@echo "        make clean                   # Remove .d .o files and compiled program" >&2
	@echo " " >&2

#Inform user of makefile usage
all:
	@echo "Copying files..."
	cp ../conf/LoLCOM /etc/system.conf.d
	mkdir -p /tmp
	rm -rf /tmp/*
	cp -rp ../resources /tmp
	@echo "Finished"

#Addition to clean so it removes .d files too
clean:
	rm *.d .depend

COMPILER_TYPE= gnu

CC= gcc

PROG= LoLCOM
SRCS= LoLCOM.c vbe.c video_gr.c keyboard.c timer.c logic.c helper.c RTC.c mouse.c UART.c speaker.c

CCFLAGS= -Wall -O3

#Toggle debug mode
.if "${DEBUG}" == "1"
CFLAGS+= -DDEBUG
.endif

DPADD+= ${LIBDRIVER} ${LIBSYS} 
LDADD+= -llm -ldriver -lsys

LDFLAGS+= -L .

MAN=

.include <bsd.gcc.mk>
.include <bsd.prog.mk>