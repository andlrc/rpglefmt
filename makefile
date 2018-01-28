FEATURES	= -DFEAT_ICEBREAK
CFLAGS		= -Wall -Werror -Wextra $(FEATURES)

all:	rpglefmt README
.PHONY:	all

rpglefmt.o:	rpglefmt.c rpglefmt.h fmt.h
fmt.o:		fmt.c rpglefmt.h fmt.h dclstore.h
dclstore.o:	dclstore.c rpglefmt.h fmt.h dclstore.h

rpglefmt: rpglefmt.o fmt.o dclstore.o

test:	rpglefmt
	cd ./t && ./run-tests
.PHONY:	test

README:	rpglefmt.1
	LC_ALL=C MANWIDTH=80 man -l $< > $@

clean:
	-rm rpglefmt.o fmt.o dclstore.o
	-rm rpglefmt
.PHONY:	clean
