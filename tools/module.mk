HOST_SOURCES += \
	tools/read_boot.c \
	tools/set_boot.c \
	tools/fat16.c \
	tools/fat16_test.c \
	tools/img_edit.c

TARGETS += \
	tools/read_boot.exe \
	tools/set_boot.exe \
	tools/fat16_test.exe \
	tools/img_edit.exe

INTERMEDIATES += $(HOST_SOURCES:.c=.o)

tools/read_boot.o: tools/fat16.h
tools/fat16.o: tools/fat16.h
tools/fat16_test.o: tools/fat16.h
tools/img_edit.o: tools/fat16.h

tools/read_boot.exe: tools/read_boot.o
	$(HOST_CC) -o $@ $^

tools/set_boot.exe: tools/set_boot.o
	$(HOST_CC) -o $@ $^

tools/fat16_test.exe: tools/fat16_test.o tools/fat16.o
	$(HOST_CC) -o $@ $^

tools/img_edit.exe: tools/img_edit.o tools/fat16.o
	$(HOST_CC) -o $@ $^
