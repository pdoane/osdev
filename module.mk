TARGETS += os.img

os.img: tools/img_edit.exe boot/boot.bin boot/loader.bin kernel/kernel.bin
	tools/img_edit.exe create os.img boot/boot.bin
	tools/img_edit.exe add os.img boot/loader.bin
	tools/img_edit.exe add os.img kernel/kernel.bin
