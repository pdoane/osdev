CC := gcc
CFLAGS := -std=c99 -Wall -O

TARGETS := read_boot.exe set_boot.exe

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

read_boot.exe: read_boot.c fat16.h
	$(CC) -o $@ $^ $(CFLAGS)

set_boot.exe: set_boot.c
	$(CC) -o $@ $^ $(CFLAGS)
