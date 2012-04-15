all: all_targets

TARGETS :=
INTERMEDIATES :=

include ./module.mk
include boot/module.mk
include tools/module.mk

CC := gcc
CFLAGS := -std=c99 -Wall -O

%.bin: %.asm
	nasm -f bin -o $@ $<

all_targets: $(TARGETS)

clean:
	rm -f $(TARGETS) $(INTERMEDIATES)
