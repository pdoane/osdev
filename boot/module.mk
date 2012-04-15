TARGETS += \
	boot/boot.bin \
	boot/loader.bin

boot/boot.bin: boot/defines.asm
boot/loader.bin: boot/defines.asm
