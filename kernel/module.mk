SOURCES := \
	kernel/kernel.c

OBJECTS := $(SOURCES:.c=.o)

TARGETS += kernel/kernel.bin
INTERMEDIATES += $(OBJECTS)

kernel/kernel.bin: $(OBJECTS) kernel/linker.ld
	$(LD) -T kernel/linker.ld -o $@ $(OBJECTS)
