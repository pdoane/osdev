TARGETS += \
	tools/read_boot.exe \
	tools/set_boot.exe \
	tools/fat16_test.exe \
	tools/img_edit.exe

INTERMEDIATES += \
	tools/read_boot.o \
	tools/set_boot.o \
	tools/fat16.o \
	tools/fat16_test.o \
	tools/img_edit.o

tools/read_boot.o: tools/fat16.h
tools/fat16.o: tools/fat16.h
tools/fat16_test.o: tools/fat16.h
tools/img_edit.o: tools/fat16.h

tools/read_boot.exe: tools/read_boot.o
	$(CC) -o $@ $^

tools/set_boot.exe: tools/set_boot.o
	$(CC) -o $@ $^

tools/fat16_test.exe: tools/fat16_test.o tools/fat16.o
	$(CC) -o $@ $^

tools/img_edit.exe: tools/img_edit.o tools/fat16.o
	$(CC) -o $@ $^
