all: boot.bin

clean:
	rm -f boot.bin

%.bin: %.asm
	nasm -f bin -o $@ $<
