#include <stdlib.h>
#include <stdio.h>

#pragma pack(1)
#define debug(a) printf a

void OSD_Puts(char *s);
//#include "../swap.c"

int MEM_read(unsigned long a) { return 0; }
#include "../misc.c"
#include "../minfat.c"

#define WRITECOPY
#include "common.h"

void TEST_resetdir(void) {
  // FIXME - changedir 0 doesn't change back to root
  current_directory_cluster = 0;
  ChangeDirectory(NULL);
}

char TestLoad_filename[13];
DIRENTRY *TestLoad_p = NULL;

int TestLoad_findfile(DIRENTRY *p) {
  char str[13];
  memcpy(str, p->Name, 8);
  memcpy(&str[9], p->Extension, 3);
  str[8] = '.';
  str[12] = '\0';

  if (!strcmp(str, TestLoad_filename)) {
    TestLoad_p = p;
    return 1;
  }
  return 0;
}

unsigned char safeRom[16384];

/************************************************************************************/
void testCreateDir(void) {
  TEST_resetdir();
  DirCd("ZX");

  DirMkdir("ABC");
  DirCd("ABC");
  nr_entries = 0;
  dir(show);
  passifeq(nr_entries, 2, "got abc");

  TEST_resetdir();
  DirCd("ZX");
  DirRm("ABC");
  DirCd("ABC");
  nr_entries = 0;
  dir(show);
  passifeq(nr_entries, 33, "dir removed");
}

/************************************************************************************/
void testRemoveDir(void) {
  TEST_resetdir();
	passif(FindDrive(), "init drive");
  // passif(DirMkdir("DIRECT1"), "create");
  // passif(DirRm("DIRECT1"), "remove");

  passif(DirMkdir("DIRECT2"), "create");
  passif(DirCd("DIRECT2"), "change to it");
  passif(FileCreate("FILE", 1024), "create a file");
  TEST_resetdir();
  passif(!DirRm("DIRECT2"), "cannot remove");

  passif(DirCd("DIRECT2"), "change to it");
  passif(FileRm("FILE"), "delete file");
  TEST_resetdir();
  passif(DirRm("DIRECT2"), "can remove due to all files removed");
}

/************************************************************************************/
void testCreateFile(void) {
  TEST_resetdir();
  DirCd("ZX");

  DirMkdir("ABC");
  DirCd("ABC");
  nr_entries = 0;
  dir(show);
  passifeq(nr_entries, 2, "got abc");

  char testFile[] = "TESTFILE.DAT";

  passif(FileCreate(testFile, 32768), "file is created");

  makeRom(32768);
  writeFile(testFile, 32768);
  passif(TestSave(testFile), "save file");
  clearBuff();

  TestLoad(testFile);
  passif(compareFile(testFile), "file has been written properly");

  passif(FileRm(testFile), "check file can be removed");
}

/************************************************************************************/
void testFilenames(void) {
  char str[256];

  FilenameNormalise(str, "ZX");
  passifstreq(str, "ZX         ", "fill out spaces, no ext");

  FilenameNormalise(str, "XX.C");
  passifstreq(str, "XX      C  ", "fill out spaces, short ext");

  FilenameNormalise(str, "XX.TXT");
  passifstreq(str, "XX      TXT", "fill out spaces, normal ext");

  FilenameNormalise(str, "caesar.tzx");
  passifstreq(str, "CAESAR  TZX", "fill out spaces, normal ext, lowercase");

  FilenameNormalise(str, "AUTOEXEC.BAT");
  passifstreq(str, "AUTOEXECBAT", "Large filename");

  FilenameNormalise(str, "OS-200!.ROM");
  passifstreq(str, "OS-200! ROM", "with symbols and stuff");
}

/************************************************************************************/
void testReserveSpace(void) {
  passif(FindDrive(), "should be ok");
  unsigned long cluster = ReserveSpaceFat(65536, 0);
  printf("space reserved at cluster %08lX\n", cluster);
  passif(cluster > 0, "valid cluster returned");
  passifeq(FreeSpaceFat(cluster), 65536/(cluster_size*512), "free space");
}

/************************************************************************************/
//TODO remove
void testFreeSpace(void) {
  fileTYPE file;
  // openCard();
  passif(FindDrive(), "should be ok");


  passif(FileOpen(&file, "SMELK3~1OLD"), "Find file to free up");

  printf("free space at cluster %08lX\n", file.cluster);

  passifeq(FreeSpaceFat(file.cluster), 16384/(cluster_size*512), "free space");



  // closeCard();
}

/************************************************************************************/
void testFileCreate(void) {
  char fn2[13];
  // openCard();
  passif(FindDrive(), "should be ok");

  char fn[] = "hello.dat";
  passif(!FileExists(fn), "should not exist");
  passif(FileCreate(fn, 65536), "creates file");
  passif(FileExists(fn), "should now exist");

  // try to overwrite - fail
  passif(!FileCreate(fn, 65536), "fail cos already exists");

  // write loads of files into empty directory
  TEST_resetdir();
  DirCd("empty");

  for (int i=0; i<32; i++) {
    sprintf(fn2, "file.%d", i);
    passif(FileCreate(fn2, 512), "creates file");
  }

  TEST_resetdir();
  passif(DirMkdir("newdir"), "can make dir");
  passif(DirCd("newdir"), "can change dir");
  nr_entries = 0;
  dir(show);
  passifeq(nr_entries, 2, "dir changed");

  passif(DirMkdir("newdir"), "can make dir");
  passif(DirCd("newdir"), "can change dir");
  nr_entries = 0;
  dir(show);
  passifeq(nr_entries, 2, "dir changed");

  // closeCard();
}


/************************************************************************************/
// void DirEnum(void *pv, int (*cb)(void *, DIRENTRY *, char *));
void testRemoveFileEntry(void) {
  fileTYPE file;
  // openCard();
  passif(FindDrive(), "should be ok");
  passif(FileExists("SMELK3~1.OLD"), "check file exists");
  passif(FileRm("SMELK3~1.OLD"), "file can be removed");
  passif(!FileExists("SMELK3~1.OLD"), "check file is removed");
  // closeCard();
}

void testLegacy() {
  buff = (unsigned char *) malloc(64*1024);


  // testFilenames();
  // testCreateDir();
  // testCreateFile();

// void FilenameNormalise(char *out, const char *in);

  // passif(!FindDrive(), "should fail without disk open");
  passif(sizeof(struct MasterBootRecord) == 512, "checking structures");

  //openCard();
  passif(FindDrive(), "should pass when disk open");
  debug(("dir_entries = %d\n", dir_entries));
#if FAT==32
  passif(IsFat32(), "should be fat32");
#else
  passif(!IsFat32(), "should be fat16");
#endif
  // TODO: extended filename
  TEST_resetdir();
  passif(cd("zx"), "chdir zx");
  TEST_resetdir();
  passif(cd("ZX"), "chdir zx");

  TEST_resetdir();
  passif(cd("ZX         "), "chdir zx");
  passif(!cd("ZX         "), "chdir zx should fail when in directory");
  TEST_resetdir();

  nr_entries = 0;
  dir(show);
  printf("current_directory_cluster = %u\n", current_directory_cluster);
  passifeq(nr_entries, 7, "got root dir");
  passif(cd("ZX         "), "chdir zx");
  //
  printf("current_directory_cluster = %u\n", current_directory_cluster);
  nr_entries = 0;
  dir(show);
  passifeq(nr_entries, 33, "got zx dir");

  TEST_resetdir();
  // dir(show);
  passif(TestLoad("BASIC2  ROM"), "load file");
  passif(!compareFile("../../../../../acorn-electron/roms/Basic2.rom"), "load file succeeded");

  makeRom(16384);
  writeFile("test.rom", 16384);

  // load rom from fs, save to file, compare to wrong rom - make sure it doesn't match
  passif(TestLoad("SMELK3~1OLD"), "load file");
  writeFile("test2.rom", 16384);
  passif(compareFile("test.rom"), "Files are not the same");
  memcpy(safeRom, buff, 16384);

  // write alternative rom to buffer, try to save, load file back, check it with alternative rom on disk
  makeRom(16384);
  passif(TestSave("SMELK3~1OLD"), "should be able to write file");
  passif(TestLoad("SMELK3~1OLD"), "load file");
  passif(!compareFile("test.rom"), "Files are the same");

  // write original rom back to disk
  memcpy(buff, safeRom, 16384);
  passif(TestSave("SMELK3~1OLD"), "should be able to write file");
  passif(TestLoad("SMELK3~1OLD"), "load file");
  passif(compareFile("test.rom"), "Files are not the same");
  passif(!compareFile("test2.rom"), "Files are the same");

  // test deeper subdirs
  TEST_resetdir();
  DirCd("ZX        ");
  nr_entries = 0;
  dir(show);
  passifeq(nr_entries, 33, "got zx dir");
  DirCd("ROMS      ");
  nr_entries = 0;
  dir(show);
  passifeq(nr_entries, 6, "got zx/roms dir");

  // closeCard();
  free(buff);
}

void testMkdirRmdir() {
  // openCard();
  passif(FindDrive(), "should pass when disk open");

  DirCd("empty");
  dir(show);

  // closeCard();
  // info();
}

#define test(a) fprintf(stderr, "-----------------\nTESTING " # a "\n-----------------\n"); printf("-----------------\nTESTING " # a "\n-----------------\n"); test##a()


void testGetFreeCluster() {
  long sectors_read_last = sectors_read;
  passifeq(GetFreeCluster(first_free_cluster), 32819, "check got free clusters");
  fprintf(stderr, "Sectors read before finding free cluster -> %d\n", sectors_read - sectors_read_last);
}


int main(int argc, char **argv) {
  openCard();
  test(Legacy);
	test(ReserveSpace);
  // test(FreeSpace);
  test(FileCreate);
  test(RemoveDir);
  test(RemoveFileEntry);
  test(CreateDir);
  test(MkdirRmdir);

  testGetFreeCluster();


	closeCard();
  info();
  return tests_run - tests_passed;
}
