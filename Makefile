all: all_targets check

SOURCES :=
TARGETS :=
TESTS :=

include ./module.mk
include boot/module.mk
include kernel/module.mk
include tools/module.mk

# Standard Settings
CC := gcc
STD_CFLAGS := -I . -std=c99 -Wall -Werror-implicit-function-declaration -fno-common -mno-red-zone -O2

# Cross Compiler
CROSS_CC := x86_64-elf-gcc
CROSS_CFLAGS := -DCROSS -fno-builtin -nostdlib -nostartfiles -nodefaultlibs $(STD_CFLAGS)
CROSS_LD := x86_64-elf-ld

# Host Compiler without standard library
HOST_CFLAGS := -DHOST -fno-builtin -nostdlib -nostartfiles -nodefaultlibs $(STD_CFLAGS)

# Host Compiler for unit testing without standard library
TEST_CFLAGS := -DTEST -fno-builtin -nostdlib -nostartfiles -nodefaultlibs $(STD_CFLAGS)

# Host Compiler with standard library
NATIVE_CFLAGS := -DNATIVE $(STD_CFLAGS)

%.bin: %.asm
	nasm -f bin -o $@ $<

%.cross.o: %.asm
	nasm -f elf64 -o $@ $<

%.cross.o: %.c
	$(CROSS_CC) -c -MD $(CROSS_CFLAGS) $< -o $@

%.host.o: %.c
	$(CC) -c -MD $(HOST_CFLAGS) $< -o $@

%.test.o: %.c
	$(CC) -c -MD $(TEST_CFLAGS) $< -o $@

%.native.o: %.c
	$(CC) -c -MMD $(NATIVE_CFLAGS) $< -o $@

%.exe.run: %.exe
	$^
	@echo > $@

all_targets: $(TARGETS)

check: $(TESTS:=.run)

usb_e: tools/set_boot.exe boot/boot.bin boot/loader.bin kernel/kernel.bin
	tools/set_boot.exe //./e: boot/boot.bin
	cp boot/loader.bin e:/
	cp kernel/kernel.bin e:/

clean:
	rm -f */*.bin */*.exe */*.exe.run */*.o */*.d

-include $(SOURCES:.c=.host.d)
-include $(SOURCES:.c=.cross.d)
-include $(SOURCES:.c=.test.d)
-include $(SOURCES:.c=.native.d)
