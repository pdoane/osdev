#!/bin/bash
set -ex
qemu-system-x86_64 -hda os.img -smp cores=4,threads=2 -usbdevice mouse -usbdevice keyboard
#sudo qemu-system-x86_64 -hda os.img -smp cores=4,threads=2 -net nic -net tap,ifname=mac -usbdevice mouse -usbdevice keyboard
