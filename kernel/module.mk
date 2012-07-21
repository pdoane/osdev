# NOTE - kernel.c must come first

KERNEL_SOURCES := \
	kernel/kernel.c \
	acpi/acpi.c \
	console/console.c \
	console/cmd.c \
	cpu/detect.c \
	cpu/smp.c \
	gfx/gfx.c \
	gfx/vga.c \
	input/input.c \
	input/keymap.c \
	intr/except.c \
	intr/idt.c \
	intr/intr.c \
	intr/ioapic.c \
	intr/local_apic.c \
	intr/pic.c \
	mem/mem_dump.c \
	mem/vm.c \
	net/addr.c \
	net/arp.c \
	net/buf.c \
	net/checksum.c \
	net/dhcp.c \
	net/dns.c \
	net/eth.c \
	net/icmp.c \
	net/intel.c \
	net/intf.c \
	net/ipv4.c \
	net/ipv6.c \
	net/loopback.c \
	net/net.c \
	net/ntp.c \
	net/port.c \
	net/rlog.c \
	net/route.c \
	net/tcp.c \
	net/udp.c \
	pci/driver.c \
	pci/pci.c \
	pci/registry.c \
	stdlib/format.c \
	stdlib/link.c \
	stdlib/string.c \
	time/pit.c \
	time/rtc.c \
	time/time.c \
	usb/ehci.c \
	usb/uhci.c \
	usb/usb.c \
	usb/controller.c \
	usb/desc.c \
	usb/dev.c \
	usb/driver.c \
	usb/hub.c \
	usb/kbd.c \
	usb/mouse.c

KERNEL_ASM := \
	intr/interrupt.asm

SOURCES += $(KERNEL_SOURCES)
ASM_SOURCES += $(KERNEL_ASM)
TARGETS += kernel/kernel.bin

KERNEL_OBJECTS := $(KERNEL_SOURCES:.c=.cross.o) $(KERNEL_ASM:.asm=.cross.o)

kernel/kernel.bin: $(KERNEL_OBJECTS) kernel/linker.ld
	$(CROSS_LD) -Map kernel/kernel.map -T kernel/linker.ld -o $@ $(KERNEL_OBJECTS)

# -------------------------------------------------------------------------------------------------
# Tests

SOURCES += \
	console/console_mock.c \
	console/console_test.c \
	net/tcp_test.c \
	stdlib/format_test.c \
	stdlib/string_test.c

TESTS += \
	console/console_test.exe \
	net/tcp_test.exe \
	stdlib/format_test.exe \
	stdlib/string_test.exe \
	stdlib/format_test_native.exe \
	stdlib/string_test_native.exe

TCP_TEST_SOURCES := \
	net/addr.c \
	net/buf.c \
	net/checksum.c \
	net/intf.c \
	net/port.c \
	net/route.c \
	net/tcp.c \
	net/tcp_test.c \
	test/test.c \
	time/time.c

console/console_test.exe: test/test.test.o console/console_test.test.o console/console.test.o
	$(CC) -o $@ $^

net/tcp_test.exe: $(TCP_TEST_SOURCES:.c=.test.o)
	$(CC) -o $@ $^

stdlib/format_test.exe: test/test.test.o stdlib/format_test.test.o stdlib/format.test.o
	$(CC) -o $@ $^

stdlib/string_test.exe: test/test.test.o stdlib/string_test.test.o stdlib/string.test.o
	$(CC) -o $@ $^

stdlib/format_test_native.exe: test/test.native.o stdlib/format_test.native.o
	$(CC) -o $@ $^

stdlib/string_test_native.exe: test/test.native.o stdlib/string_test.native.o
	$(CC) -o $@ $^
