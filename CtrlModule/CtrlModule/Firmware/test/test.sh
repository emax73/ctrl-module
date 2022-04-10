#!/bin/bash
testsuite()
{
  echo =================
  echo TESTING $@
  echo =================
}

make clean
make || exit 1

testsuite FAT32
./testminfat32 > testminfat32.txt || exit 1
testsuite FAT16
./testminfat16 > testminfat16.txt || exit 1
# testsuite FAT12
# ./testminfat12 > testminfat12.txt || exit 1
testsuite STORAGE
./teststorage > teststorage.txt || exit 1
# testsuite TAPE
# ./testtape > testtape.txt || exit 1
testsuite SWAP
./testswap > testswap.txt || exit 1
testsuite MISC
./testmisc > testmisc.txt || exit 1
testsuite DISK
./testdisk > testdisk.txt || exit 1
testsuite DISK256
./testdisk256 > testdisk256.txt || exit 1

echo -e "\n\nAll tests pass."
