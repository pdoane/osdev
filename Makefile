all: all_targets

SOURCES :=
ASM_SOURCES :=
TARGETS :=

include ./module.mk
include boot/module.mk
include kernel/module.mk
include tools/module.mk

HOST_CC := gcc
HOST_CFLAGS := -I . -std=c99 -Wall -O2

CC := x86_64-elf-gcc
CFLAGS := -I . -std=c99 -fno-builtin -nostdlib -nostartfiles -nodefaultlibs -Wall -O2
LD := x86_64-elf-ld

%.bin: %.asm
	nasm -f bin -o $@ $<

%.cross.o: %.asm
	nasm -f elf64 -o $@ $<

%.host.o: %.c
	$(HOST_CC) -MMD -c $(HOST_CFLAGS) $< -o $@

%.cross.o: %.c
	$(CC) -MD -c $(CFLAGS) $< -o $@

all_targets: $(TARGETS)

usb_e: tools/set_boot.exe boot/boot.bin boot/loader.bin kernel/kernel.bin
	tools/set_boot.exe //./e: boot/boot.bin
	cp boot/loader.bin e:/
	cp kernel/kernel.bin e:/

clean:
	rm -f \
		$(TARGETS) \
		$(ASM_SOURCES:.asm=.cross.o) \
		$(SOURCES:.c=.host.o) \
		$(SOURCES:.c=.cross.o) \
		$(SOURCES:.c=.host.d) \
		$(SOURCES:.c=.cross.d)

-include $(SOURCES:.c=.host.d)
-include $(SOURCES:.c=.cross.d)
