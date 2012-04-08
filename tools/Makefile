CC := gcc
CFLAGS := -std=c99 -Wall -O

all: diskwrite.exe

clean:
	rm -f diskwrite.exe

diskwrite.exe: diskwrite.c
	$(CC) -o $@ $< $(CFLAGS)
