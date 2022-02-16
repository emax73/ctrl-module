CtrlModule for MJSpec + others
==============================

This is the control module based on: https://github.com/robinsonb5/CtrlModuleTutorial

This builds for the MJ board which has a JTAG board created
from a RPi Pico RP2040.  It has onboard EEPROM to store multiple images rather than replace the EEPROM
on the JTAG board.  For the most part, this code is adapted for MJ board, which is a QMTech XC6LX25, and
a daughter card with VGA, DeltaSigma DACs, tape in, SdCard, and keyboard / mouse.

Files
=====
Script/x - is the bash script I use for building the projects - I don't use the ISE platform, always
preferred command line.  Sure, a makefile would be better, maybe in future.

Building
========

To build the ZPU code, use `./build.sh.`  Use `script/x c` to build the code, `script/x r` to download 
via Altera (yes altera) UsbBlaster using UrJtag.

Disclaimer: I use linux only in my work, so have not considered the need for Windows as yet.  I'm
sure I'll get around to it.  Some mess exists, but since it's not a production ready piece of code
I'm happy no-one will suffer too badly from a little code mess.  I'll get around to it eventually.


