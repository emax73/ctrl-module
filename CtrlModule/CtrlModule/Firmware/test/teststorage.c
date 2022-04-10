#include <stdlib.h>
#include <stdio.h>

#define USE_LOADFILE
#define LOAD_INITIAL_ROM
#define INCLUDE_FILE_REMOVE

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
int MEM_read(unsigned long a) { return 0; }
#include "../misc.c"
#include "../minfat.c"
#define UNDER_TEST
#include "../storage.c"
#define WRITECOPY
#define LOADSAVE_WRAPPERS
#include "common.h"

int nrProgressBar = 0;
int progressBarBits = 0;
void OSD_ProgressBar(int v,int bits) {
  printf("OSD_ProgressBar: v = %d bits = %d\n", v, bits);
  nrProgressBar ++;
  progressBarBits = bits;
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

// #undef HW_HOST
// #define HW_HOST(r) HW_HOST_LL2(r)
// int Save(const char *fn, long len) {
//   fileTYPE file;
//   int i;
//   unsigned long data;
//   long remaining = len;
//
//   if (FileOpen(&file, fn)) {
//     debug(("found file\n"));
//     while (remaining > 0) {
//       for (i=0; i<512; i+=4) {
//         data = HW_HOST(REG_HOST_EXTDATA);
//         sector_buffer[i+0] = (data >> 24) & 0xff;
//         sector_buffer[i+1] = (data >> 16) & 0xff;
//         sector_buffer[i+2] = (data >> 8) & 0xff;
//         sector_buffer[i+3] = data & 0xff;
//       }
//       FileWrite(&file, sector_buffer);
//       FileNextSector(&file);
//       remaining -= 512;
//     }
//     return 1;
//   }
//
//   return 0;
// }


int write_sector_callback(unsigned char *data) {
  memset(data, 0x55, 512);
  return 1;
}

int sector_count_10s = 0;
int write_10sectors_callback(unsigned char *data) {
  if (sector_count_10s < 10) {
    memset(data, 0x55, 512);
    sector_count_10s++;
    return 1;
  }
  return 0;
}


int main(int argc, char **argv) {
  openCard();
  buff = (unsigned char *) malloc(64*1024);

  passifeq(GetBits(16384), 5, "check getbits");

  passif(FindDrive(), "should pass when disk open");
  passif(IsFat32(), "should be fat32");
  regREG_HOST_BOOTDATATYPE = 0xdeadbeef;
  progressBarBits = nrProgressBar = 0;
  passif(LoadROM("BASIC2  ROM"), "read rom");
  passif(nrProgressBar != 0, "check progress bar was used");
  passifeq(progressBarBits, 5, "progress bar dimentions correct");

  passifeq(regREG_HOST_ROMSIZE, 16384, "should load 16k rom");
  passifeq(cntREG_HOST_BOOTDATA, 4096, "should write 4k 32bit words");
  passifeq(regREG_HOST_BOOTDATATYPE, TYPE_TAPE, "should be set to tape type");
  passifeq(cntREG_HOST_BOOTDATATYPE, 1, "should be set one time");

  makeRom(65536);
  writeFile("64krom.bin", 65536);
  // clearBuff();

  // regREG_HOST_EXTDATA = 0x01020304;
  passif(FileCreate("BASIC3.ROM", 65536), "create file");
  passif(Save("BASIC3  ROM", 65536), "save success");
  passifeq(cntREG_HOST_EXTDATA, 65536/4, "read data");

  TestLoad("BASIC3  ROM");

  passif(!compareFile("64krom.bin"), "make sure file was written correctly");


  unsigned char *buff2 = (unsigned char *) malloc(64*1024);

  progressBarBits = nrProgressBar = 0;
  passifeq(65536, LoadFileX("BASIC3  ROM", buff2), "can read to buffer");
  memcpy(buff, buff2, 65536);
  passif(!compareFile("64krom.bin"), "make sure file was read correctly");
  passif(nrProgressBar != 0, "check progress bar was used");
  passifeq(progressBarBits, 7, "progress bar dimentions correct");

  progressBarBits = nrProgressBar = 0;
  passif(SaveFileX("BASIC4.ROM", buff2, 65536), "file saved successfully");
  passif(nrProgressBar != 0, "check progress bar was used");
  passifeq(progressBarBits, 7, "progress bar dimentions correct");

  progressBarBits = nrProgressBar = 0;
  passif(SaveFileX("BASIC4.ROM", buff2, 65536), "file overwritten successfully");
  passif(nrProgressBar != 0, "check progress bar was used");
  passifeq(progressBarBits, 7, "progress bar dimentions correct");

  progressBarBits = nrProgressBar = 0;
  passifeq(65536, LoadFileX("BASIC4.ROM", buff), "can read to buffer");
  passif(nrProgressBar != 0, "check progress bar was used");
  passif(!compareFile("64krom.bin"), "make sure file was read correctly");
  passifeq(progressBarBits, 7, "progress bar dimentions correct");

  int old_sectors_written;
  
  old_sectors_written = sectors_written;
  SaveFile("tempfile.tmp", NULL, 128*1024);
  passifeq(sectors_written-old_sectors_written, 8, "Just allocated space using SaveFile");
  FileRm("tempfile.tmp");

  old_sectors_written = sectors_written;
  SaveFile("tempfile.tmp", write_sector_callback, 128*1024);
  passifeq(sectors_written-old_sectors_written, 8+256, "wrote to image file using SaveFile");
  FileRm("tempfile.tmp");

  old_sectors_written = sectors_written;
  sector_count_10s = 0;
  SaveFile("tempfile.tmp", write_10sectors_callback, 128*1024);
  passifeq(sectors_written-old_sectors_written, 8+10, "wrote to image file using SaveFile");
  FileRm("tempfile.tmp");
  
  
  //   write_sector_callback
  
  
//     callbackData = data;
// 
//     int SaveFile(const char *fn, void (*callback)(unsigned char *data), long len) {
//   return SaveFile(filename, callbacksave, len);
// #endif
  
  
  free(buff);
  free(buff2);
  closeCard();
  info();
  return tests_run - tests_passed;
}
