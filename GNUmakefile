PROJECT=c-usip
VERSION=1.0.0
PREFIX=/usr/local
PROGS=usip$(EXE)
LIBS=libusip.a
CC=gcc -Wall -std=c99
##
all: $(PROGS) $(LIBS)
clean:
	rm -f $(PROGS) $(LIBS) *.o
install:
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 644 $(LIBS) $(DESTDIR)$(PREFIX)/lib
	install -d $(DESTDIR)$(PREFIX)/include
	install -m 644 usip.h $(DESTDIR)$(PREFIX)/include
##
usip$(EXE): main.c libusip.a
	$(CC) $(LDFLAGS) -o $@ $^
libusip.a: usip.o
	$(AR) $(ARFLAGS) $@ $^
##
%.o: %.c usip.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<
