ARCHIVER=ar
ARFLAGS=rcs
CC=gcc
CFLAGS=-c
OFILES=smallwin.o
OUTNAME=libsmallwin.a

all: smallwin

smallwin:
	$(CC) $(CFLAGS) smallwin.c
	$(ARCHIVER) $(ARFLAGS) $(OUTNAME) $(OFILES)

clean:
	rm -rf *.o
