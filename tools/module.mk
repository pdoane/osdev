SOURCES += \
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

tools/read_boot.exe: tools/read_boot.host.o
	$(HOST_CC) -o $@ $^

tools/set_boot.exe: tools/set_boot.host.o
	$(HOST_CC) -o $@ $^

tools/fat16_test.exe: tools/fat16_test.host.o tools/fat16.host.o
	$(HOST_CC) -o $@ $^

tools/img_edit.exe: tools/img_edit.host.o tools/fat16.host.o
	$(HOST_CC) -o $@ $^
