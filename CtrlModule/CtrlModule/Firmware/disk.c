#include "disk.h"
#include "minfat.h"
#include "interrupts.h"
#include "misc.h"
#include "host.h"

#define HW_DISK_CR_ACK      1
#define HW_DISK_CR_SACK     16
#define HW_DISK_CR_ERR      8
#define HW_DISK_READSECTOR0  0x20000
#define HW_DISK_READSECTOR1  0x40000

#define HW_DISK_RESET   0x10000

#define HW_DISK_WRITESECTOR0 0x100000
#define HW_DISK_WRITESECTOR1 0x200000

#define HW_DISK_SR_BLOCKNR   0xffff
#define NOREQ   0xffffffff
#define DISK_BLOCK_NUMBER_MASK 0x7fffffff
#define DISK_BLOCK_WRITE 0x80000000

#ifndef UNDER_TEST
#define HW_DISK_SR_R() *(volatile unsigned int *)(0xFFFFFA10)
#define HW_DISK_CR_W(d) *(volatile unsigned int *)(0xFFFFFA10) = d
#define HW_DISK_DATA_R() *(volatile unsigned int *)(0xFFFFFA14)
#define HW_DISK_DATA_W(d) *(volatile unsigned int *)(0xFFFFFA14) = d
#endif

unsigned long disk_cr = 0;

#ifndef BLOCK_SIZE
#define BLOCK_SIZE  512
#endif

#ifdef SAMCOUPE
#define MAX_BLOCK_NR 1600
#define NR_DISK_LBA   1600
#define SECTOR_COUNT_512 10
#define SECTOR_COUNT_512_CPM 9
#define SECTOR_COUNT_256 20
#else
#define MAX_BLOCK_NR 720
#define NR_DISK_LBA   360
#define SECTOR_COUNT_512 9
#define SECTOR_COUNT_256 18
#endif

#if BLOCK_SIZE == 512
#ifdef SAMCOUPE
#define STS_TO_BLOCK(side, track, sector) (((((track)*2)+(side))*(isCPM ? SECTOR_COUNT_512_CPM : SECTOR_COUNT_512))+(sector)-1)
#define STS_TO_BLOCK_CPM(side, track, sector) (((((track)*2)+(side))*9)+(sector)-1)
#else
#define STS_TO_BLOCK(side, track, sector) ((side)*80+(track*SECTOR_COUNT_512)+(sector))
#endif
#define BLOCK_OFFSET(sector) (0)
#elif BLOCK_SIZE == 256
#define STS_TO_BLOCK(side, track, sector) (((side)*80+(track*SECTOR_COUNT_256)+(sector))>>1)
#define BLOCK_OFFSET(sector) ((sector)&1) ? BLOCK_SIZE : 0
#endif
#define STS(side, track, sector) ((((side)&1)<<12)|(((track)&0x7f)<<5)|((sector)&0x1f))

#ifndef debug
#define debug(a) {}
#endif

static unsigned char diskIsInserted[NR_DISKS] = {0, 0};
static unsigned long diskReadBlock[NR_DISKS] = {NOREQ, NOREQ};
static unsigned long diskWriteBlock[NR_DISKS] = {NOREQ, NOREQ};
static unsigned short diskReadOffset[NR_DISKS] = {0, 0};
unsigned long laststs = 0;

#ifdef SAMCOUPE
int isCPM = 0;
#endif

unsigned long StsToBlock(unsigned long sts) {
  unsigned char track = (sts >> 5) & 0x7f;
  unsigned char sector = sts & 0x1f;
  unsigned char side = (sts >> 12) & 1;

  laststs = sts;

  return STS_TO_BLOCK(side, track, sector);
}

void DiskSignalsHandler(void) {

  unsigned long sr = HW_DISK_SR_R();
  if (sr & (HW_DISK_READSECTOR0|HW_DISK_WRITESECTOR0)) {
    diskReadBlock[0] = StsToBlock(sr & HW_DISK_SR_BLOCKNR);
    diskReadOffset[0] = BLOCK_OFFSET(sr & HW_DISK_SR_BLOCKNR);
    if (sr & HW_DISK_WRITESECTOR0) diskReadBlock[0] |= DISK_BLOCK_WRITE;
  }
  if (sr & (HW_DISK_READSECTOR1|HW_DISK_WRITESECTOR1)) {
    diskReadBlock[1] = StsToBlock(sr & HW_DISK_SR_BLOCKNR);
    diskReadOffset[1] = BLOCK_OFFSET(sr & HW_DISK_SR_BLOCKNR);
    if (sr & HW_DISK_WRITESECTOR1) diskReadBlock[1] |= DISK_BLOCK_WRITE;
  }

  if (sr & HW_DISK_RESET) {
    disk_cr &= ~(HW_DISK_CR_SACK|HW_DISK_CR_ERR);
    HW_DISK_CR_W(disk_cr);
  }
}

unsigned int diskLba[NR_DISKS][NR_DISK_LBA];

void DiskHandlerSingle(int disk) {
  unsigned long mem, lba;
  int i;
  if (diskReadBlock[disk] != NOREQ) {
    // disk_cr ^= 0x40000000;

    if ((diskReadBlock[disk] & DISK_BLOCK_NUMBER_MASK) > MAX_BLOCK_NR) {
      diskReadBlock[disk] = NOREQ;
      disk_cr |= HW_DISK_CR_SACK|HW_DISK_CR_ERR;
      return;
    }

    lba = diskLba[disk][diskReadBlock[disk]];

    debug(("lba %08X\n", lba));

    // first read the sector
    if (lba < MaxLba() && sd_read_sector(lba, sector_buffer)) {
      if (diskReadBlock[disk] & DISK_BLOCK_WRITE) {
        // write to half of sector then write whole sector
        for (i=0; i<BLOCK_SIZE; i++) {
          sector_buffer[i + diskReadOffset[disk]] = HW_DISK_DATA_R();
          HW_DISK_DATA_W(0x200);
          HW_DISK_DATA_W(0x000);
        }
        // TODO ENABLE
        sd_write_sector(lba, sector_buffer);
        debug(("write good\n"));
      } else {
        // read
        for (i=0; i<BLOCK_SIZE; i++) {
          HW_DISK_DATA_W(sector_buffer[i + diskReadOffset[disk]] | 0x100);
          HW_DISK_DATA_W(sector_buffer[i + diskReadOffset[disk]] | 0x000);
        }
        debug(("read good\n"));
      }

      disk_cr |= HW_DISK_CR_SACK;
    } else {
      disk_cr |= HW_DISK_CR_SACK|HW_DISK_CR_ERR;
    }
    diskReadBlock[disk] = NOREQ;
  }
}

void DiskHandler(void) {
  DiskSignalsHandler();
  DiskHandlerSingle(0);
  DiskHandlerSingle(1);
  HW_DISK_CR_W(disk_cr);
}

static unsigned int mem = 0, disk=0;
static void LbaCallback(unsigned int lba) {
  diskLba[disk][mem++] = lba;
}

int DiskOpen(int i, const char *fn) {
  disk = i;
  mem = 0;

  int len = Open(fn, LbaCallback);
  diskIsInserted[i] = len ? 1 : 0;
#ifdef SAMCOUPE
  isCPM = len == 737280;
#endif
  return len ? 1 : 0;
}

int DiskClose(void) {
  diskIsInserted[0] = 0;
  return 0;
}

void DiskInit(void) {
	int i, j;
	for (i=0; i<NR_DISKS; i++) {
		diskIsInserted[i] = 0;
		diskReadBlock[i] = NOREQ;
		diskWriteBlock[i] = NOREQ;
		diskReadOffset[i] = 0;
		for (j=0; j<NR_DISK_LBA; j++) {
			diskLba[i][j] = 0;
		}
	}
	mem = 0;
	disk = 0;
	disk_cr = 0;
	laststs = 0;
}
