#include "misc.h"
#include "tape.h"
#include "host.h"
#include "interrupts.h"

#ifndef DISABLE_TAPE
static unsigned int loadLen;
#ifdef SAVE_BUFFER
static unsigned int saveLen;
static unsigned int savePos;
static unsigned int saveBlockLen;
char tapeSaveSize[32] = " - 0x00000000 bytes";
#endif
char tapeLoadSize[32] = " - 0x00000000 bytes";

static int buffer = 0;
static unsigned char tape_buffer[2][512];
static int loadBlockNr = -1;

static int hyperloadBlock = -1;
static int nextHyperloadBlock = 0;
static int doTapeRewind = 0;


void TapeUpdateStatus(void) {
  intToString(&tapeLoadSize[3], loadLen);
#ifdef SAVE_BUFFER
  intToString(&tapeSaveSize[3], savePos);
#endif
}

#ifndef UNDER_TEST
#define HW_TAPE_R() *(volatile unsigned int *)(0xFFFFFA00)
#define HW_TAPE_W(a) *(volatile unsigned int *)(0xFFFFFA00) = a

#define HW_HTAPE_R() *(volatile unsigned int *)(0xFFFFFA04)
#define HW_HTAPE_W(a) *(volatile unsigned int *)(0xFFFFFA04) = a

#ifdef SAVE_BUFFER
#define HW_TAPEIN_R() *(volatile unsigned int *)(0xFFFFFA04)
#define HW_TAPEIN_W(a) *(volatile unsigned int *)(0xFFFFFA04) = a
#endif
#endif

#define TAPE_W_DACK 0x100
#define TAPE_W_DEND 0x200

#define HTAPE_W_HDCLK 0x400
#define HTAPE_W_HBUSY 0x800
#define HTAPE_W_HRESET 0x1000

#define TAPE_R_HRESET   0x8
#define TAPE_R_DRACK	0x4
#define TAPE_R_RESET 	0x2
#define TAPE_R_DREQ 	0x1

#define HTAPE_R_HREQ   0x1

#ifdef SAVE_BUFFER
#define TAPE_IN_W_ACK 		0x01
#define TAPE_IN_R_DVALID 	0x100
#define TAPE_IN_R_DEND 		0x200

void SaveHandler(void) {
  unsigned int tapeInStatus = HW_TAPEIN_R();
  if (tapeInStatus & TAPE_IN_R_DVALID) {
    MEM_write(MEMPOS_SAVE_TAPE | savePos + saveBlockLen + 2, tapeInStatus & 0xff);
    saveBlockLen ++;
  }
  if (tapeInStatus & TAPE_IN_R_DEND) {
    MEM_write(MEMPOS_SAVE_TAPE | savePos, saveBlockLen & 0xff);
    MEM_write(MEMPOS_SAVE_TAPE | savePos + 1, saveBlockLen >> 8);
    savePos += 2 + saveBlockLen;
    saveBlockLen = 0;
  }
  HW_TAPEIN_W(TAPE_IN_W_ACK);
}
#endif

static unsigned int nrLoadBytes = 0;
static unsigned long loadPos = 0;

int TapeGetByte();

static void TapeISR(void) {
	unsigned int sr = HW_TAPE_R();

	if (sr & TAPE_R_DREQ) {
    if (nrLoadBytes == 0) {
      int ch = TapeGetByte();
      nrLoadBytes = ch | (TapeGetByte() << 8);
      HW_TAPE_W(TAPE_W_DEND | TAPE_W_DACK);
    } else {
      HW_TAPE_W(TapeGetByte() | TAPE_W_DACK);
      nrLoadBytes --;
    }
    if (loadPos >= loadLen) TapeStop();
	}

  if (sr & TAPE_R_DRACK) {
    HW_TAPE_W(0);
	}

  if (sr & TAPE_R_HRESET) {
    doTapeRewind = 1;
  }
}

#define MAX_TAPE_LBA    512
static unsigned int tapeLbaNr = 0;
static unsigned int tapeLba[MAX_TAPE_LBA];

static unsigned int mem = 0;
static void LbaCallback(unsigned int lba) {
  if (tapeLbaNr < MAX_TAPE_LBA)
    tapeLba[tapeLbaNr++] = lba;
}

int TapeLoad(const char *fn) {
  tapeLbaNr = 0;
  loadLen = Open(fn, LbaCallback);
  nrLoadBytes = loadPos = 0;

  TapeRewind();
  return loadLen;
}

#ifdef SAVE_BUFFER
int saveCallbackPtr = 0;
static void TapeSaveCallback(unsigned char *data) {
  int i;
  for (i=0; i<512; i++) {
    data[i] = MEM_read(MEMPOS_LOAD_TAPE | saveCallbackPtr ++);
  }
}

int TapeSave(void) {
  char fn[13], fn2[13];
  int n = 0;

  GuessFilename(fn, MEMPOS_SAVE_TAPE, "TAPE");

  strcpy(fn2, fn);
  strcat(fn2, ".TAP");
  while (FileExists(fn2)) {
    MutateFilename(fn, n++);
    strcpy(fn2, fn);
    strcat(fn2, ".TAP");
  }

  saveCallbackPtr = 0;
  return SaveFile(fn2, TapeSaveCallback, savePos);
}

void TapeErase(void) {
  saveLen = 0;
  savePos = 0;
  saveBlockLen = 0;
}
#endif /* SAVE_BUFFER */

void TapeLoadBlock() {
  if (doTapeRewind) {
    TapeRewind();
    doTapeRewind = 0;
  }

  if (loadBlockNr >= 0) {
    if (loadBlockNr >= tapeLbaNr) {
      loadBlockNr = 0;
    }
    TapeReadSector(loadBlockNr, tape_buffer[loadBlockNr&1]);
    loadBlockNr = -1;
  }

  if ((HW_HTAPE_R() & HTAPE_R_HREQ) && hyperloadBlock < 0) {
    int i;
    HW_HTAPE_W(HTAPE_W_HBUSY|HTAPE_W_HRESET);

    hyperloadBlock = nextHyperloadBlock++;
    if (hyperloadBlock >= tapeLbaNr) {
      nextHyperloadBlock = 0;
      hyperloadBlock = nextHyperloadBlock++;
    }

    HW_HTAPE_W(HTAPE_W_HBUSY);
    TapeReadSector(hyperloadBlock, tape_buffer[0]);
    for (i=0; i<512; i++) {
      HW_HTAPE_W(HTAPE_W_HBUSY|HTAPE_W_HDCLK|tape_buffer[0][i]);
      HW_HTAPE_W(HTAPE_W_HBUSY|tape_buffer[0][i]);
    }
    HW_HTAPE_W(0);
    hyperloadBlock = -1;
  }
  }

int TapeGetByte() {
  int ch = tape_buffer[buffer][loadPos & 511];
  loadPos ++;

  if ((loadPos & 511) == 0) {
    loadBlockNr = (loadPos >> 9) + 1;
    buffer = (loadPos & 512) ? 1 : 0;
  }
  return ch;
}

int TapeReadSector(unsigned int sect, unsigned char *buff) {
  if (sect >= tapeLbaNr) {
    return 0;
  }
  unsigned int lba = tapeLba[sect];
  if (lba < MaxLba() && sd_read_sector(lba, buff)) {
    return 1;
  }
  return 0;
}

void TapeRewind(void) {
  nrLoadBytes = loadPos = 0;
  buffer = 0;
  nextHyperloadBlock = 0;
  HW_HTAPE_W(HTAPE_W_HRESET);
  HW_HTAPE_W(0);

  TapeReadSector(0, tape_buffer[0]);
  TapeReadSector(1, tape_buffer[1]);
  loadBlockNr = -1;
}

#ifdef SAVE_BUFFER
void TapeUseRecordBuffer(void) {
  int i;
  for (i=0; i<savePos; i++) {
    MEM_write(MEMPOS_LOAD_TAPE+i, MEM_read(MEMPOS_SAVE_TAPE+i));
  }
  loadLen = savePos;
  TapeRewind();
}
#endif

void TapeInit(void) {
  loadBlockNr = -1;
  buffer = 0;
  loadLen = 0;
  nextHyperloadBlock = 0;
  tapeLbaNr = 0;
#ifdef SAVE_BUFFER
  saveLen = 0;
  savePos = 0;
  saveBlockLen = 0;
#endif

  SetIntHandler(IRQ_TAPE, &TapeISR);
#ifdef SAVE_BUFFER
	SetIntHandler(IRQ_TAPEIN, &SaveHandler);
#endif
}

#endif // disable tape
