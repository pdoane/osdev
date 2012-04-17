# NOTE - kernel.c must come first

KERNEL_SOURCES := \
	kernel/kernel.c \
    kernel/idt.c \
	kernel/format.c

KERNEL_ASM := \
	kernel/interrupt.asm

TEST_SOURCES := \
	kernel/format_test.c

SOURCES += $(KERNEL_SOURCES) $(TEST_SOURCES)
ASM_SOURCES += $(KERNEL_ASM)
TARGETS += \
	kernel/kernel.bin \
	kernel/format_test.exe \
	kernel/format_test_host.exe

KERNEL_OBJECTS := $(KERNEL_SOURCES:.c=.cross.o) $(KERNEL_ASM:.asm=.cross.o)

kernel/kernel.bin: $(KERNEL_OBJECTS) kernel/linker.ld
	$(LD) -T kernel/linker.ld -o $@ $(KERNEL_OBJECTS)

kernel/format_test.exe: kernel/format_test.host.o kernel/format.host.o
	$(HOST_CC) -o $@ $^

kernel/format_test_host.exe: kernel/format_test.host.o
	$(HOST_CC) -o $@ $^

kernel/interrupt.asm: kernel/keymap.asm
