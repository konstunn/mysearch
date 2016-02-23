
CC?=gcc
CFLAGS?=-Wall -g
CFLAGS+=-std=gnu99
DESTDIR?=/
PREFIX?=usr

TARGETS=mysearch

all: $(TARGETS) 

mysearch: mysearch.c
	$(CC) $(CFLAGS) mysearch.c -o mysearch -lpthread

client: client.c
	$(CC) $(CFLAGS) client.c -o client

test: test.c
	$(CC) $(CFLAGS) test.c -o test

clean:
	-@rm -vf $(TARGETS) 

.PHONY: install clean
