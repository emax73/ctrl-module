#ifndef HOST_H
#define HOST_H

#define HOSTSTATE 0xFFFFFFF0
#define DEBUGSTATE 0xFFFFFFF4

#define HOSTMEM  0xA0000000
#define HOSTMEM_TAPELOAD    HOSTMEM
#define HOSTMEM_TAPESAVE    (HOSTMEM+0x80000)
#define HOSTBASE 0xFFFFFFE0
#ifndef HW_HOST
#define HW_HOST(x) *(volatile unsigned int *)(HOSTBASE+x)
#endif

/* SPI registers */

/* Host Boot Data register */
#define REG_HOST_BOOTDATA 0x08

/* JOY Keystrokes register */
#define REG_HOST_JOYKEY 0x0D

/* Host control register */
#define REG_HOST_CONTROL 0x0C
#define HOST_CONTROL_RESET 1
#define HOST_CONTROL_DIVERT_KEYBOARD 2
#define HOST_CONTROL_DIVERT_SDCARD 4
#define HOST_CONTROL_SELECT 8
#define HOST_CONTROL_START 16

/* DIP switches / "Front Panel" controls - bits 15 downto 0 */
#define REG_HOST_SCALERED 0x10
#define REG_HOST_SCALEGREEN 0x14
#define REG_HOST_ROMSIZE 0x18
#define REG_HOST_SW 0x1C

/* External read memory device */
#define REG_HOST_EXTADDR    0x1D
#define REG_HOST_EXTDATA    0x1E
#define REG_HOST_EXTTYPE    0x1F
#define REG_HOST_BOOTDATATYPE   0x1B

#define TYPE_TAPE     0x00

// allowing 128k for this stuff - probably way too much
// reduced memory requirement.
#define MEMPOS_LOAD_TAPE       0x60000
#define MEMPOS_SAVE_TAPE       0x60000
#define MEMPOS_ROM_POS         0x80000
#define DISK0_POS              0x50000
#define DISK1_POS              0x54000
#define DISK0_SECTBUFF_POS     0x58000
#define DISK1_SECTBUFF_POS     0x58200

// build options
//#define SAVE_BUFFER

#endif
