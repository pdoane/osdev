all: all_targets

SOURCES :=
ASM_SOURCES :=
TARGETS :=

include ./module.mk
include boot/module.mk
include kernel/module.mk
include tools/module.mk

# Cross Compiler
CROSS_CC := x86_64-elf-gcc
CROSS_CFLAGS := -I . -std=c99 -fno-builtin -nostdlib -nostartfiles -nodefaultlibs -Wall -O2
CROSS_LD := x86_64-elf-ld

# Host Compiler without standard library
HOST_CC := gcc
HOST_CFLAGS := -I . -std=c99 -fno-builtin -nostdlib -nostartfiles -nodefaultlibs -Wall -O2

# Host Compiler with standard library
NATIVE_CC := gcc
NATIVE_CFLAGS := -I . -std=c99 -Wall -O2

%.bin: %.asm
	nasm -f bin -o $@ $<

%.cross.o: %.asm
	nasm -f elf64 -o $@ $<

%.cross.o: %.c
	$(CROSS_CC) -c -MD -DCROSS $(CROSS_CFLAGS) $< -o $@

%.host.o: %.c
	$(HOST_CC) -c -MD -DHOST $(HOST_CFLAGS) $< -o $@

%.native.o: %.c
	$(NATIVE_CC) -c -MMD -DNATIVE $(NATIVE_CFLAGS) $< -o $@

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
		$(SOURCES:.c=.native.o) \
		$(SOURCES:.c=.host.d) \
		$(SOURCES:.c=.cross.d) \
		$(SOURCES:.c=.native.d)

-include $(SOURCES:.c=.host.d)
-include $(SOURCES:.c=.cross.d)
-include $(SOURCES:.c=.native.d)
