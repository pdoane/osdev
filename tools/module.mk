SOURCES += \
	tools/fat16.c \
	tools/fat16_test.c \
	tools/img_edit.c \
	tools/os_helper.c \
	tools/read_boot.c \
	tools/set_boot.c

TARGETS += \
	tools/img_edit.exe \
	tools/os_helper.exe \
	tools/read_boot.exe \
	tools/set_boot.exe

tools/img_edit.exe: tools/img_edit.native.o tools/fat16.native.o
	$(CC) -o $@ $^

tools/os_helper.exe: tools/os_helper.native.o
	$(CC) -o $@ $^ -lws2_32

tools/read_boot.exe: tools/read_boot.native.o
	$(CC) -o $@ $^

tools/set_boot.exe: tools/set_boot.native.o
	$(CC) -o $@ $^



TESTS += \
	tools/fat16_test.exe

tools/fat16_test.exe: test/test.native.o tools/fat16_test.native.o tools/fat16.native.o
	$(CC) -o $@ $^
