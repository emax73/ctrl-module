#include <stdlib.h>
#include <stdio.h>

#define SAVE_BUFFER

unsigned char *loadBuff;
unsigned char *saveBuff;
// unsigned char *buff;
// unsigned char *buff2;

void TapeStop() {}

unsigned int TAPE_SR = 0;
unsigned int HW_TAPE_R() {
  return TAPE_SR;
}
unsigned int TAPE_W = 0;

void HW_TAPE_W(unsigned int a) {
  TAPE_W = a;
}

unsigned int TAPEIN_R = 0;
unsigned int TAPEIN_W = 0;

unsigned int HW_TAPEIN_R(void) {
  return TAPEIN_R;
}
void HW_TAPEIN_W(unsigned int r) {
  TAPEIN_W = r;
}

unsigned char *tapeData;

#define MEM_read(a) tapeData[a]
#define MEM_write(a, b) tapeData[a] = b

#pragma pack(1)
#define debug(a) printf a

void OSD_Puts(char *s);
void OSD_ProgressBar(int v,int bits);

unsigned long cntREG_HOST_CONTROL = 0;
unsigned long cntREG_HOST_BOOTDATA = 0;
unsigned long cntREG_HOST_ROMSIZE = 0;
unsigned long cntREG_HOST_EXTADDR = 0;
unsigned long cntREG_HOST_EXTDATA = 0;
unsigned long cntREG_HOST_DATATYPE = 0;
unsigned long cntREG_HOST_BOOTDATATYPE = 0;
unsigned long regREG_HOST_CONTROL;
unsigned long regREG_HOST_BOOTDATA;
unsigned long regREG_HOST_ROMSIZE;
unsigned long regREG_HOST_EXTADDR;
unsigned long regREG_HOST_EXTDATA;
unsigned long regREG_HOST_DATATYPE;
unsigned long regREG_HOST_BOOTDATATYPE;

unsigned long HW_HOST_LL(int reg, unsigned long prev) {
  printf("HW_HOST(reg = %X, prev = %08lX)\n", reg, prev);
  return prev;
}

unsigned long reg;

extern unsigned long HW_HOST_LL2(int reg);
#define HW_HOST(h) HW_HOST_LL(h, reg##h); cnt##h ++; reg##h
#include "../misc.h"
#include "../misc.c"
#include "../minfat.c"
#define UNDER_TEST
#include "../storage.c"
#define WRITECOPY
#define LOADSAVE_WRAPPERS

#include "common.h"
#include "../tape.c"
#undef memcpy
#undef memset
#undef strcpy
#undef strcat

void OSD_ProgressBar(int v,int bits) {
  printf("OSD_ProgressBar: v = %d bits = %d\n", v, bits);
}


int wptr = 0;
unsigned long HW_HOST_LL2(int reg) {
  if (REG_HOST_EXTDATA == reg) {
    unsigned long data = (buff[wptr+0]<<24)|(buff[wptr+1]<<16)|(buff[wptr+2]<<8)|buff[wptr+3];
    wptr += 4;
    printf("wptr %d data %08lX\n", wptr-4, data);
    cntREG_HOST_EXTDATA++;
    return data;
  }
  return 0;
}

typedef void (*handler_t)(void);

static handler_t handlers[IRQ_MAX] = {0};

void SetIntHandler(int irq, void (*new_handler)()) {
  handlers[irq] = new_handler;
}

unsigned long chksum(unsigned long sum, unsigned char b) {
  return ((sum << 8) | (sum >> 24)) ^ b;
}

unsigned long consumebytes(int n) {
  unsigned long csum = 0;
  int nr = 0;
  for(;;) {
    handlers[IRQ_TAPE]();
    TapeLoadBlock();
    if ((TAPE_W&(TAPE_W_DACK|TAPE_W_DEND)) == (TAPE_W_DACK|TAPE_W_DEND)) break;
    csum = chksum(csum, TAPE_W);
    nr ++;
  }
  printf("consumebytes: consumed %d bytes\n", nr);
  passifeq(n, nr, "amount of data matches expectation");
  return csum;
}

void testLoad() {
  passifeq(loadLen, 0, "check nothing loaded");
  TapeLoad("HUNCHB  TAP");
  passifeq(loadLen, 43520, "check hunchback loaded");

  handlers[IRQ_TAPE]();
  passifeq(nrLoadBytes, 0, "check no action if no flag raised");

  TAPE_SR = TAPE_R_DREQ;
  handlers[IRQ_TAPE]();
  passifeq(nrLoadBytes, 19, "check no action if no flag raised");
  passifeq(TAPE_W, TAPE_W_DACK|TAPE_W_DEND, "check initial data end");

  int n = 19, ndacks = 0;
  unsigned long csum = 0;
  while (n--) {
    handlers[IRQ_TAPE]();
    TapeLoadBlock();
    csum = chksum(csum, TAPE_W);
    if ((TAPE_W&(TAPE_W_DACK|TAPE_W_DEND)) == TAPE_W_DACK) ndacks++;
  }
  passifeq(ndacks, 19, "not got end for 19 bytes");
  passifeqh(csum, 0x55506366, "chksum header");

  handlers[IRQ_TAPE]();
  TapeLoadBlock();
  passifeq(TAPE_W&(TAPE_W_DACK|TAPE_W_DEND), TAPE_W_DACK|TAPE_W_DEND, "got end");
  passifeq(nrLoadBytes, 164, "program");
  passifeqh(consumebytes(nrLoadBytes), 0xdeaef181, "chksum program");

  passifeq(nrLoadBytes, 19, "header");
  passifeqh(consumebytes(nrLoadBytes), 0xff49982e, "chksum header");

  passifeq(nrLoadBytes, 6914, "screen$");
  passifeqh(consumebytes(nrLoadBytes), 0x561091d7, "chksum screen$");

  passifeq(nrLoadBytes, 19, "header");
  passifeqh(consumebytes(nrLoadBytes), 0x40ec0ea2, "chksum header");

  passifeq(nrLoadBytes, 36302, "data$");
  passifeqh(consumebytes(nrLoadBytes), 0xe1d5af9b, "chksum data$");
}

void testInit() {
  passifeq(handlers[IRQ_TAPE], NULL, "check no isrs installed");
  passifeq(handlers[IRQ_TAPEACK], NULL, "check no isrs installed");
  passifeq(handlers[IRQ_TAPEIN], NULL, "check no isrs installed");
  TapeInit();
  passifeq(handlers[IRQ_TAPE], LoadHandler, "check isrs installed");
  passifeq(handlers[IRQ_TAPEACK], LoadHandlerAck, "check isrs installed for save");
}

void testSave() {
  passifeq(handlers[IRQ_TAPEIN], SaveHandler, "check isrs installed");
  TAPEIN_R = TAPEIN_W = 0;

  handlers[IRQ_TAPEIN]();
  passifeq(TAPE_IN_W_ACK, TAPEIN_W, "ack set on interrupt");
  TAPEIN_R = TAPEIN_W = 0;
  passifeq(savePos, 0, "No difference in save position");
  passifeq(saveBlockLen, 0, "No difference in save block size");

  TAPEIN_R = 0xaa | TAPE_IN_R_DVALID;
  handlers[IRQ_TAPEIN]();
  passifeq(TAPE_IN_W_ACK, TAPEIN_W, "ack set on interrupt");
  TAPEIN_R = TAPEIN_W = 0;
  passifeq(savePos, 0, "No difference in save position");
  passifeq(saveBlockLen, 1, "increase in block size");

  for (int i=0; i<255; i++) {
    TAPEIN_R = i | TAPE_IN_R_DVALID;
    handlers[IRQ_TAPEIN]();
  }
  TAPEIN_R = TAPEIN_W = 0;
  passifeq(savePos, 0, "No difference in save position");
  passifeq(saveBlockLen, 256, "increase in block size");

  TAPEIN_R = TAPE_IN_R_DEND;
  handlers[IRQ_TAPEIN]();
  passifeq(TAPE_IN_W_ACK, TAPEIN_W, "ack set on interrupt");
  TAPEIN_R = TAPEIN_W = 0;
  passifeq(savePos, 258, "Block saved");
  passifeq(saveBlockLen, 0, "block size back to zero");

  TAPEIN_R = 0xdd | TAPE_IN_R_DEND | TAPE_IN_R_DVALID;
  handlers[IRQ_TAPEIN]();
  passifeq(TAPE_IN_W_ACK, TAPEIN_W, "ack set on interrupt");
  TAPEIN_R = TAPEIN_W = 0;
  passifeq(savePos, 261, "Block saved");
  passifeq(saveBlockLen, 0, "block size back to zero");

  passif(!memcmp(buff2, "\x00\x01\xaa", 3), "first block header");
  int nr = 0;
  for (int i=0; i<255; i++) {
    if (buff2[3+i] == i) nr++;
  }
  passifeq(nr, 255, "block contents sorted");

  hexdump(buff2, savePos);
}

int main(void) {
  tapeData = (unsigned char *) malloc(512*1024);
  loadBuff = tapeData + MEMPOS_LOAD_TAPE;
  saveBuff = tapeData + MEMPOS_SAVE_TAPE;
  buff = loadBuff;
  buff2 = saveBuff;
  openCard();
  passif(FindDrive(), "should pass when disk open");
  DirCd("ZX");
  // dir(show);

  testInit();
  testLoad();
  testSave();
  // testFilenameDerivation();

  free(tapeData);
  closeCard();
  info();
  return tests_run - tests_passed;
}
