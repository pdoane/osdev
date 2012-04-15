all: all_targets

HOST_SOURCES :=
TARGETS :=
INTERMEDIATES :=

include ./module.mk
include boot/module.mk
include kernel/module.mk
include tools/module.mk

HOST_CC := gcc
HOST_CFLAGS := -std=c99 -Wall -O2

CC := x86_64-elf-gcc
CFLAGS := -std=c99 -nostdlib -nostartfiles -nodefaultlibs -Wall -O2
LD := x86_64-elf-ld

%.bin: %.asm
	nasm -f bin -o $@ $<

$(HOST_SOURCES:.c=.o): %.o: %.c
	$(HOST_CC) -c $(HOST_CFLAGS) $< -o $@

all_targets: $(TARGETS)

clean:
	rm -f $(TARGETS) $(INTERMEDIATES)
