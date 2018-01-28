FEATURES	= -DFEAT_ICEBREAK
CFLAGS		= -Wall -Werror -Wextra $(FEATURES)

all:	rpglefmt README test
.PHONY:	all

rpglefmt.o:	rpglefmt.h fmt.h rpglefmt.c
fmt.o:		dclstore.h rpglefmt.h fmt.c
dclstore.o:	dclstore.h dclstore.c

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
