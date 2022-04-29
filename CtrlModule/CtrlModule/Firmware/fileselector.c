#include "minfat.h"
#include "menu.h"

static int romindex=0;
static int romcount;

// function prototypes
static void listroms();
static void selectrom(int row);
static void scrollroms(int row);
static DIRENTRY *nthfile(int n);

int (*loadfunction)(const char *filename, DIRENTRY *p); // Callback function

// menu declarations
static char romfilenames[13][30];

static struct menu_entry rommenu[]=
{
	{MENU_ENTRY_CALLBACK,romfilenames[0],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[1],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[2],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[3],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[4],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[5],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[6],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[7],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[8],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[9],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[10],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[11],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_CALLBACK,romfilenames[12],MENU_ACTION(&selectrom)},
	{MENU_ENTRY_SUBMENU,"Back",MENU_ACTION(0)},
	{MENU_ENTRY_NULL,0,MENU_ACTION(scrollroms)}
};


static void copyname(char *dst,const unsigned char *src,int l)
{
	int i;
	for(i=0;i<l;++i)
		*dst++=*src++;
	*dst++=0;
}


static void selectrom(int row)
{
	DIRENTRY *p=nthfile(romindex+row);
	if(p)
	{
		copyname(longfilename,p->Name,11);	// Make use of the long filename buffer to store a temporary copy of the filename,
											// since loading it by name will overwrite the sector buffer which currently contains it!
		if(loadfunction)
			(*loadfunction)(longfilename, p);
	}
}


static void selectdir(int row)
{
	DIRENTRY *p=nthfile(romindex+row);
	if(p)
		ChangeDirectory(p);
	romindex=0;
	listroms();
	Menu_Draw();
}


static void scrollroms(int row)
{
	switch(row)
	{
		case ROW_LINEUP:
			if(romindex)
				--romindex;
			break;
		case ROW_PAGEUP:
			romindex-=12;
			if(romindex<0)
				romindex=0;
			break;
		case ROW_LINEDOWN:
			++romindex;
			break;
		case ROW_PAGEDOWN:
			romindex+=12;
			break;
	}
	listroms();
	Menu_Draw();
}



typedef struct {
  int romindex;
  int i;
} ListRomsType;

static int listroms_cb(void *p, DIRENTRY *d, char *lfn, int index, int prevlfn) {
  ListRomsType *lrt = (ListRomsType *)p;
  if (lrt->romindex) {
    lrt->romindex--;
    
  } else if (lrt->i < 12) {
    if(d->Attributes&ATTR_DIRECTORY) {
      rommenu[lrt->i].action=MENU_ACTION(&selectdir);
      romfilenames[lrt->i][0]=16; // Right arrow
      romfilenames[lrt->i][1]=' ';
      if(longfilename[0])
        copyname(romfilenames[lrt->i++]+2,longfilename,28);
      else
        copyname(romfilenames[lrt->i++]+2,d->Name,11);
    } else {
      rommenu[lrt->i].action=MENU_ACTION(&selectrom);
      if(longfilename[0])
        copyname(romfilenames[lrt->i++],longfilename,28);
      else
        copyname(romfilenames[lrt->i++],d->Name,11);
    }
  } else {
    return 1; // stop listing
  }
  return 0;
}

static void listroms() {
  ListRomsType lrt = {romindex, 0};
  int i;
    
  // fill new files in
	DirEnum((void*)&lrt, listroms_cb, 0);

    // clear out rom names
  for (; lrt.i<12; lrt.i++) {
    romfilenames[lrt.i][0]=0;
  }
}

// select nth file from current directory
typedef struct {
  int index;
  DIRENTRY *d;
} NthFileType;

static int nthfile_cb(void *p, DIRENTRY *d, char *lfn, int index, int prevlfn) {
  int result = 0;
  NthFileType *nf = (NthFileType *)p;
  if (nf->index) nf->index--;
  else {
    nf->d = d;
    result = 1;
  }
  return result;
}

static DIRENTRY *nthfile(int n) {
  NthFileType nf = {n, 0};
	DirEnum((void*)&nf, nthfile_cb, 0);
  return nf.d;
}

void FileSelector_Show(int row)
{
	romindex=0;
	listroms();
	rommenu[13].action=MENU_ACTION(Menu_Get()); // Set parent menu entry
	Menu_Set(rommenu);
}


void FileSelector_SetLoadFunction(int (*func)(const char *filename, DIRENTRY *p))
{
	loadfunction=func;
}

