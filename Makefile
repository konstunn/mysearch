
CC=g++
CFLAGS?=-Wall -g
#CFLAGS+=-std=gnu99
DESTDIR?=/
PREFIX?=usr
TARGETS=mysearch

all: $(TARGETS) 

mysearch: mysearch.c
	$(CC) $(CFLAGS) mysearch.c -o mysearch

client: client.c
	$(CC) $(CFLAGS) client.c -o client

test: test.c
	$(CC) $(CFLAGS) test.c -o test

#install: cmux
	#install -m 0755 cmux $(DESTDIR)/$(PREFIX)/bin/cmux
	# TODO: just add current directory to PATH

clean:
	-@rm -vf $(TARGETS) 

.PHONY: install clean
