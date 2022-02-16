#!/bin/bash
losetup -P /dev/loop55 ./tmp/cardw.$1
mount /dev/loop55p1 t
cd t
bash
cd ..

umount t

fsck.vfat /dev/loop55p1

losetup -d /dev/loop55
