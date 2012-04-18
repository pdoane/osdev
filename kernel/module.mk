# NOTE - kernel.c must come first

KERNEL_SOURCES := \
	kernel/kernel.c \
	kernel/console.c \
    kernel/idt.c \
	kernel/format.c \
	kernel/keyboard.c \
	kernel/pic.c \
	kernel/string.c \
	kernel/vga.c \
	kernel/vm.c

KERNEL_ASM := \
	kernel/interrupt.asm

TEST_SOURCES := \
	kernel/format_test.c \
	kernel/string_test.c \
	kernel/test.c

SOURCES += $(KERNEL_SOURCES) $(TEST_SOURCES)
ASM_SOURCES += $(KERNEL_ASM)
TARGETS += \
	kernel/kernel.bin \
	kernel/test.exe \
	kernel/test_native.exe

KERNEL_OBJECTS := $(KERNEL_SOURCES:.c=.cross.o) $(KERNEL_ASM:.asm=.cross.o)

kernel/kernel.bin: $(KERNEL_OBJECTS) kernel/linker.ld
	$(CROSS_LD) -T kernel/linker.ld -o $@ $(KERNEL_OBJECTS)

kernel/test.exe: $(TEST_SOURCES:.c=.host.o) kernel/format.host.o kernel/string.host.o
	$(HOST_CC) -o $@ $^

kernel/test_native.exe: $(TEST_SOURCES:.c=.native.o)
	$(NATIVE_CC) -o $@ $^

kernel/interrupt.asm: kernel/keymap.asm
