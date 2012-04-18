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

tools/read_boot.exe: tools/read_boot.native.o
	$(NATIVE_CC) -o $@ $^

tools/set_boot.exe: tools/set_boot.native.o
	$(NATIVE_CC) -o $@ $^

tools/fat16_test.exe: tools/fat16_test.native.o tools/fat16.native.o
	$(NATIVE_CC) -o $@ $^

tools/img_edit.exe: tools/img_edit.native.o tools/fat16.native.o
	$(NATIVE_CC) -o $@ $^
