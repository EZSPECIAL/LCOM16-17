# Makefile for lab5

#Addition to clean so it removes .d files too
clean:
	rm *.d .depend

COMPILER_TYPE= gnu

CC= gcc

PROG= lab5
SRCS= lab5.c test5.c vbe.c video_gr.c stbi_wrapper.c

CFLAGS= -Wall

DPADD+= ${LIBDRIVER} ${LIBSYS} 
LDADD+= -llm -ldriver -lsys

LDFLAGS+= -L .

MAN=

.include <bsd.gcc.mk>
.include <bsd.prog.mk>