# NOTE - kernel.c must come first

KERNEL_SOURCES := \
	kernel/kernel.c \
	kernel/acpi.c \
	kernel/arp.c \
	kernel/console.c \
	kernel/eth_8254x.c \
	kernel/format.c \
	kernel/icmp.c \
	kernel/idt.c \
	kernel/ioapic.c \
	kernel/ipv4.c \
	kernel/keyboard.c \
	kernel/keymap.c \
	kernel/local_apic.c \
	kernel/net.c \
	kernel/net_driver.c \
	kernel/pci.c \
	kernel/pci_classify.c \
	kernel/pci_driver.c \
	kernel/pic.c \
	kernel/pit.c \
	kernel/smp.c \
	kernel/string.c \
	kernel/uhci.c \
	kernel/vga.c \
	kernel/vm.c

KERNEL_ASM := \
	kernel/interrupt.asm

SOURCES += $(KERNEL_SOURCES)
ASM_SOURCES += $(KERNEL_ASM)
TARGETS += kernel/kernel.bin

KERNEL_OBJECTS := $(KERNEL_SOURCES:.c=.cross.o) $(KERNEL_ASM:.asm=.cross.o)

kernel/kernel.bin: $(KERNEL_OBJECTS) kernel/linker.ld
	$(CROSS_LD) -T kernel/linker.ld -o $@ $(KERNEL_OBJECTS)

# -------------------------------------------------------------------------------------------------
# Tests

SOURCES += \
	kernel/console_mock.c \
	kernel/console_test.c \
	kernel/format_test.c \
	kernel/keyboard_test.c \
	kernel/string_test.c

TESTS += \
	kernel/console_test.exe \
	kernel/format_test.exe \
	kernel/keyboard_test.exe \
	kernel/string_test.exe \
	kernel/format_test_native.exe \
	kernel/string_test_native.exe

kernel/console_test.exe: test/test.test.o kernel/console_test.test.o kernel/console.test.o
	$(CC) -o $@ $^

kernel/format_test.exe: test/test.test.o kernel/format_test.test.o kernel/format.test.o
	$(CC) -o $@ $^

kernel/keyboard_test.exe: test/test.test.o kernel/keyboard_test.test.o kernel/keyboard.test.o kernel/keymap.test.o kernel/console_mock.test.o
	$(CC) -o $@ $^

kernel/string_test.exe: test/test.test.o kernel/string_test.test.o kernel/string.test.o
	$(CC) -o $@ $^

kernel/format_test_native.exe: test/test.native.o kernel/format_test.native.o
	$(CC) -o $@ $^

kernel/string_test_native.exe: test/test.native.o kernel/string_test.native.o
	$(CC) -o $@ $^
