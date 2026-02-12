CC = gcc
CFLAGS = -O2 -Wall
LDFLAGS = -lX11 -lpulse

PREFIX = /usr/local

dwmbar: dwmbar.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install: dwmbar
	cp dwmbar $(PREFIX)/bin/
	chmod 755 $(PREFIX)/bin/dwmbar

clean:
	rm -f dwmbar

.PHONY: install clean
