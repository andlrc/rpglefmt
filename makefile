FEATURES	= -DFEAT_ICEBREAK
CFLAGS		= -Wall -Werror -Wextra $(FEATURES)

all:	rpglefmt README
.PHONY:	all

rpglefmt.o:	rpglefmt.h fmt.h rpglefmt.c
fmt.o:		dclstore.h rpglefmt.h fmt.c
dclstore.o:	dclstore.h dclstore.c

rpglefmt: rpglefmt.o fmt.o dclstore.o

README:	man.1
	LC_ALL=C MANWIDTH=80 man -l man.1 > README

clean:
	-rm rpglefmt.o fmt.o dclstore.o
	-rm rpglefmt
