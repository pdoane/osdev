@ECHO OFF

qemu-system-x86_64.exe -hda os.img -smp cores=4,threads=2 -net nic -net tap,ifname=tap -usbdevice keyboard
