int r;
#define USE_LOAD_ROM
#define DEBUG

#define db printf(__FILE__ "[%d]\n", __LINE__);

unsigned int SwapBBBB(unsigned int i) { return i; }
unsigned long SwapWW(unsigned long i) { return i; }
unsigned int SwapBB(unsigned int i) { return i; }

#include "commontests.h"

int sectors_read = 0;
int sectors_written = 0;

void info() {
  printf("sectors_read = %d\n", sectors_read);
  printf("sectors_written = %d\n", sectors_written);
  fprintf(stderr, "sectors_read = %d\n", sectors_read);
  fprintf(stderr, "sectors_written = %d\n", sectors_written);
  infotests();
}

#ifndef WRITECOPY
FILE *card = NULL;

void openCard(void) {
  char str[13];
  sprintf(str, "./tmp/card.%d", FAT);
  card = fopen(str, "rwb+");
}

void closeCard(void) {
  fclose(card);
}

int sd_init() {
  if (card) return 1;
  return 0;
}

int sd_read_sector(unsigned long lba,unsigned char *buf) {
  fseek(card, lba*512, SEEK_SET);
  if ((ftell(card) / 512) == lba) {
    if (fread(buf, 1, 512, card) == 512) {
      printf ("sd_read_sector: read lba %lu\n", lba);
      sectors_read ++;
      return 1;
    }
  }
  return 0;
}

int sd_write_sector(unsigned long lba,unsigned char *buf) {
  int ret;
  fseek(card, lba*512, SEEK_SET);
  if ((ftell(card) / 512) == lba) {
    if ((ret = fwrite(buf, 1, 512, card)) == 512) {
      printf ("sd_write_sector: write lba %lu\n", lba);
      sectors_written ++;
      return 1;
    }
  }
  printf ("sd_write_sector: fail writing lba %lu returns %d\n", lba, ret);
  return 0;
}
#else
unsigned char *card = NULL;
unsigned long cardSize = 0;


void openCard(void) {
  char str[255];
  sprintf(str, "./tmp/card.%d", FAT);
  FILE *f = fopen(str, "rb");

  fseek(f, 0, SEEK_END);
  cardSize = ftell(f);
  fseek(f, 0, SEEK_SET);

  card = (unsigned char *)malloc(cardSize);
  fread(card, 1024, cardSize/1024, f);
  fclose(f);
}

void closeCard(void) {
  char str[255];
  sprintf(str, "./tmp/cardw.%d", FAT);
  FILE *f = fopen(str, "wb");
  fwrite(card, 1024, cardSize/1024, f);
  fclose(f);
  free(card);
  card = NULL;
  cardSize = 0;
}

int sd_init() {
  if (card) return 1;
  return 0;
}

int sd_read_sector(unsigned long lba,unsigned char *buf) {
  if ((lba*512) > cardSize) {
    printf ("sd_read_sector: read out of bounds (lba %lu)\n", lba);
    return 0;
  }

  memcpy(buf, card+lba*512, 512);
  printf ("sd_read_sector: lba %lu\n", lba);
  sectors_read ++;
  return 1;
}

int sd_write_sector(unsigned long lba,unsigned char *buf) {
  if ((lba*512) > cardSize) {
    printf ("sd_write_sector: write out of bounds (lba %lu)\n", lba);
    return 0;
  }

  memcpy(card+lba*512, buf, 512);
  printf ("sd_write_sector: lba %lu\n", lba);
  sectors_written ++;
  return 1;
}
#endif

unsigned char *buff;
unsigned char *buff2;

void OSD_Puts(char *s) {
  printf("OSD_Puts: %s\n", s);
}

int cd(const char *dir) {
	return DirCd(dir);
}

int dircb(void *p, DIRENTRY *d, char *lfn, int index, int prevlfn) {
	return ((int (*)(DIRENTRY *))p)(d);
}

void dir(int (*cb)(DIRENTRY *)) {
	DirEnum((void*)cb, dircb, 0);
}

int nr_entries = 0;
int show(DIRENTRY *p) {
  char str[13];
  memcpy(str, p->Name, 8);
  memcpy(&str[9], p->Extension, 3);
  str[8] = '.';
  str[12] = '\0';
  printf("name:%s attr:0x%X size:%lu lfn:%s\n", str, p->Attributes, p->FileSize, longfilename);
  nr_entries ++;
  return 0;
}

void hexdump(unsigned char *data, int len) {
	int i, j;
	for (i=0; i<len; i+=16) {
		for (j=0; j<16; j++) {
			printf("%02X ", data[i+j]);
		}
    printf(" | ");
    for (j=0; j<16; j++) {
			printf("%c", data[i+j] >= ' ' ? data[i+j] : '.');
		}

		printf("\n");
	}
}

int compareFile(const char *fn) {
  FILE *f = fopen(fn, "rb");
  int diffs = -1, pos = 0, ch;
  int nrReports = 10;
  if (f) {
    diffs = 0;
    ch = fgetc(f);
    while (!feof(f)) {
      if (ch != buff[pos]) {
        if (nrReports) {
          printf("%08u: %02X != %02X\n", pos, ch, buff[pos]);
          nrReports --;
        }
        diffs ++;
      }
      pos++;
      ch = fgetc(f);
    }

    fclose(f);
  }
  return diffs;
}

void makeRom(unsigned long len) {
  for (int i = 0; i< len; i++) {
    buff[i] = i & 0xff;
  }
}

void writeFile(const char *name, int len) {
  FILE *f = fopen(name, "wb");
  if (f) {
    fwrite(buff, 1, len, f);
    fclose(f);
  }
}

void clearBuff(void) {
  memset(buff, 0, 65536);
}

int TestSave(const char *filename) {
	int result=0;
  int pos = 0;
	int opened;
  fileTYPE file;

	if((opened=FileOpen(&file,filename))) {
		int filesize=file.size;
		result=1;

		while(filesize>0) {
			if (!FileWrite(&file,buff + pos)) {
				result=0;
				filesize=512;
			}
			FileNextSector(&file);
			filesize -= 512;
      pos += 512;
		}
	}
	return(result);
}

int TestLoad(const char *filename) {
	int result=0;
  int pos = 0;
	int opened;
  // DIRENTRY file;
  fileTYPE file;

  // strcpy(TestLoad_filename, filename);
  // dir(TestLoad_findfile);
  // file = *TestLoad_p;
  // TestLoad_p = NULL;
  //
	if((opened=FileOpen(&file,filename))) {
		int filesize=file.size;
		result=1;

		while(filesize>0) {
			if (!FileRead(&file,buff + pos)) {
				result=0;
				filesize=512;
			}
			FileNextSector(&file);
			filesize -= 512;
      pos += 512;
		}
	}
	return(result);
}

#ifdef LOADSAVE_WRAPPERS
static char *callbackData;
void callbackload(char *ptr) {
  memcpy(callbackData, ptr, 512);
  callbackData += 512;
}
#define USE_LOADFILE
int LoadFileX(const char *filename, unsigned char *data) {
  callbackData = data;
  return LoadFile(filename, callbackload);
}

void callbacksave(char *ptr) {
  memcpy(ptr, callbackData, 512);
  callbackData += 512;
}
int SaveFileX(const char *filename, unsigned char *data, long len) {
  callbackData = data;
  return SaveFile(filename, callbacksave, len);
}
#endif
