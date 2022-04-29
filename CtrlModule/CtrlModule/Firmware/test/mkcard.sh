#!/bin/bash
if [ ! -e ./tmp/card.32 ]; then
	mkdir tmp &> /dev/null
	mount -t tmpfs tmpfs ./tmp
fi

mkcard()
{
dd if=/dev/zero of=./tmp/card.$1 bs=1024 count=131072
#dd if=/dev/zero of=./tmp/card.$1 bs=1024 count=98304

fdisk ./tmp/card.$1 << EOF
n
p
1


t
$2
w
EOF

GAMESSRC=${MYHOME}/Documents/sorted/retro/zxspectrum
ROMSSRC=${MYHOME}/fpga-xilinx/acorn-electron/roms/
DISKSRC=${MYHOME}/Documents/sorted/retro/opus/specdisks/
losetup -P /dev/loop55 ./tmp/card.$1
mkfs.vfat -F $1 $3 /dev/loop55p1

mkdir t
mount /dev/loop55p1 t

dd if=/dev/zero of=./t/bigfile bs=1024 count=32768
cp ${ROMSSRC}/* ./t 

mkdir t/zx
cp -v ${GAMESSRC}/*.tap ./t/zx
cp -v ${GAMESSRC}/*.tzx ./t/zx

mkdir t/zx/roms
cp ${ROMSSRC}/* t/zx/roms

mkdir t/zx/disks
cp ${DISKSRC}/sdcld*.opd t/zx/disks

mkdir t/sam
cp ${MYHOME}/fpga-xilinx/sdcard/card/samcoupe/samdos2.dsk ./t/sam
cp ${MYHOME}/fpga-xilinx/sdcard/card/samcoupe/prodos/HiSoft-C-\(V1-35\).cpm ./t/sam/cpmdisk.cpm

mkdir t/lfn
cp ${MYHOME}/fpga-xilinx/sam-coupe2/disks/seleccion/* t/lfn

mkdir t/empty
umount t

chmod 666 ./tmp/card.$1
losetup -d /dev/loop55
rmdir t
}

mkcard 32 0c "-s 2"
mkcard 16 06
#mkcard 12 01

mkdir t
