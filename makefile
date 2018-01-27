FEATURES	= -DFEAT_ICEBREAK
CFLAGS		= -Wall -Werror $(FEATURES)

all:	rpglefmt README
.PHONY:	all

rpglefmt.o:	rpglefmt.h fmt.h rpglefmt.c
fmt.o:		rpglefmt.h fmt.c

rpglefmt: rpglefmt.o fmt.o

README:	man.1
	LC_ALL=C MANWIDTH=80 man -l man.1 > README

clean:
	-rm rpglefmt.o fmt.o
	-rm rpglefmt
