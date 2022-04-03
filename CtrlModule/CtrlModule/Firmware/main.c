#include "host.h"

#include "osd.h"
#include "keyboard.h"
#include "menu.h"
#include "ps2.h"
#include "minfat.h"
#include "spi.h"
#include "fileselector.h"
#include "storage.h"
#include "interrupts.h"
#include "tape.h"
#include "disk.h"
#include "misc.h"
#include "uart.h"
#include "machinemenu.h"

fileTYPE file;

#ifdef JOYKEYS
extern int keys_p1[];
extern int keys_p2[];
#endif

int OSD_Puts(char *str)
{
	int c;
	while((c=*str++))
		OSD_Putchar(c);
	return(1);
}

void Delay()
{
	int c=16384; // delay some cycles
	while(c)
	{
		c--;
	}
}

#ifdef LOAD_INITIAL_ROM
void LoadInitialRom(void) {
	OSD_Puts("Loading ROMs...\n...from /" SYSTEM_DIR "/" ROMPAK "...");
	if (DirCd(SYSTEM_DIR)) {
		LoadROM(ROMPAK);
	} else OSD_Puts("Failed to change directory");
}
#endif

#ifdef RESET_MENU
void Reset(int row)
{
	HW_HOST(REG_HOST_CONTROL)=HOST_CONTROL_RESET|HOST_CONTROL_DIVERT_KEYBOARD; // Reset host core
	Delay();
	HW_HOST(REG_HOST_CONTROL)=HOST_CONTROL_DIVERT_KEYBOARD;
}
#endif

struct menu_entry topmenu[]; // Forward declaration.

#ifdef OPTION_MENU
static char *testpattern_labels[]=
{
	"Item 1",
	"Item 2",
	"Item 3",
	"Item 4"
};

static struct menu_entry optionsmenu[]=
{
	{MENU_ENTRY_SLIDER,"Slider",MENU_ACTION(16)},
	{MENU_ENTRY_TOGGLE,"Toggle",MENU_ACTION(0)},
	{MENU_ENTRY_CYCLE,(char *)testpattern_labels,MENU_ACTION(4)},
	{MENU_ENTRY_SUBMENU,"Exit",MENU_ACTION(topmenu)},
	{MENU_ENTRY_NULL,0,0}
};
#endif

#ifdef SWITCHES_MENU
static struct menu_entry switchesmenu[]=
{
	{MENU_ENTRY_TOGGLE,"Switch 1",MENU_ACTION(1)},
	{MENU_ENTRY_TOGGLE,"Switch 2",MENU_ACTION(2)},
	{MENU_ENTRY_TOGGLE,"Switch 3",MENU_ACTION(3)},
	{MENU_ENTRY_TOGGLE,"Switch 4",MENU_ACTION(4)},
	{MENU_ENTRY_TOGGLE,"Switch 5 (dblwnd)",MENU_ACTION(5)},
	{MENU_ENTRY_TOGGLE,"Switch 6 (wpd0)",MENU_ACTION(6)},
	{MENU_ENTRY_TOGGLE,"Switch 7 (wpd1)",MENU_ACTION(7)},
	{MENU_ENTRY_TOGGLE,"Switch 8 (upgrade spi)",MENU_ACTION(8)},
	{MENU_ENTRY_TOGGLE,"Switch 9",MENU_ACTION(9)},
	{MENU_ENTRY_TOGGLE,"Switch 10 (tp)",MENU_ACTION(10)},
	{MENU_ENTRY_TOGGLE,"Switch 11 (tbo tp)",MENU_ACTION(11)},
	{MENU_ENTRY_TOGGLE,"Switch 12 (hy tp)",MENU_ACTION(12)},
	{MENU_ENTRY_TOGGLE,"Switch 13 (tape audio)",MENU_ACTION(13)},
	{MENU_ENTRY_TOGGLE,"Switch 14 (saving)",MENU_ACTION(14)},
	{MENU_ENTRY_SUBMENU,"Back",MENU_ACTION(&topmenu)},
	{MENU_ENTRY_NULL,0,0}
};
#endif

static struct menu_entry loadfailed[];
////////////////////////////////////////////////////////////////////////////////
// TAPE HANDLING
#ifndef DISABLE_TAPE
static struct menu_entry tapestatus[];

#ifdef SAVE_BUFFER
void tapestatus_TapePlayRecordBuffer(int row) {
	TapeUseRecordBuffer();
	TapeUpdateStatus();
}

void tapestatus_TapeErase(int row) {
	TapeErase();
	TapeUpdateStatus();
}

void tapestatus_TapeSave(int row) {
	TapeSave();
}

#endif

void tapestatus_TapeRewind(int row) {
	TapeRewind();
}

void tapestatus_TapeRefreshMenu(int row) {
	TapeUpdateStatus();
	Menu_Set(tapestatus);
}

static int tapestatus_LoadTapeReal(const char *filename) {
  int result = TapeLoad(filename);
	TapeUpdateStatus();

  if(result)
		Menu_Set(tapestatus);
	else
		Menu_Set(loadfailed);
  return result;
}

void tapestatus_LoadTape(int row) {
	FileSelector_SetLoadFunction(tapestatus_LoadTapeReal);
	FileSelector_Show(row);
}

void TapeStop(void) {
	MENU_TOGGLE_VALUES &= ~(1<<10);
	HW_HOST(REG_HOST_SW)=MENU_TOGGLE_VALUES;
}

static struct menu_entry tapestatus[]=
{
	{MENU_ENTRY_SUBMENU,"Tape loaded size:",MENU_ACTION(tapestatus)},
	{MENU_ENTRY_SUBMENU,"",MENU_ACTION(tapestatus)},
#ifdef SAVE_BUFFER
	{MENU_ENTRY_SUBMENU,"Tape saved size:",MENU_ACTION(tapestatus)},
	{MENU_ENTRY_SUBMENU,"",MENU_ACTION(tapestatus)},
#endif
	{MENU_ENTRY_SUBMENU,"",MENU_ACTION(tapestatus)},
	{MENU_ENTRY_CALLBACK,"Load tape",MENU_ACTION(tapestatus_LoadTape)},
#ifdef SAVE_BUFFER
	{MENU_ENTRY_CALLBACK,"Save tape",MENU_ACTION(tapestatus_TapeSave)},
	{MENU_ENTRY_CALLBACK,"Reset tape",MENU_ACTION(tapestatus_TapeErase)},
#endif
	{MENU_ENTRY_TOGGLE,"Hyperload",MENU_ACTION(12)},
	{MENU_ENTRY_TOGGLE,"Play tape",MENU_ACTION(10)},
#ifdef SAVE_BUFFER
	{MENU_ENTRY_TOGGLE,"Save on",MENU_ACTION(14)},
#endif
	{MENU_ENTRY_CALLBACK,"Rewind tape",MENU_ACTION(tapestatus_TapeRewind)},
	{MENU_ENTRY_CALLBACK,"Refresh info",MENU_ACTION(tapestatus_TapeRefreshMenu)},
	// {MENU_ENTRY_CALLBACK,"Play recorded tape",MENU_ACTION(tapestatus_TapePlayRecordBuffer)},
	{MENU_ENTRY_SUBMENU,"Back",MENU_ACTION(&topmenu)},
	{MENU_ENTRY_NULL,0,0}
};
#endif // #ifndef DISABLE_TAPE

////////////////////////////////////////////////////////////////////////////////
// DISK HANDLING
static struct menu_entry diskstatus[];
static char diskNr = 0;
static int diskstatus_LoadDiskReal(const char *filename) {
  int result = DiskOpen(diskNr, filename);
  if(result)
		Menu_Set(diskstatus);
	else
		Menu_Set(loadfailed);
  return result;
}

void diskstatus_LoadDisk0(int row) {
	diskNr = 0;
	FileSelector_SetLoadFunction(diskstatus_LoadDiskReal);
	FileSelector_Show(row);
}

#if NR_DISKS==2
void diskstatus_LoadDisk1(int row) {
	diskNr = 1;
	FileSelector_SetLoadFunction(diskstatus_LoadDiskReal);
	FileSelector_Show(row);
}
#endif

#ifdef WRITE_E5_TO_DISK
void SaveBlankDiskCallback(unsigned char *data) {
	int i;
	for (i=0; i<512; i++) {
		data[i] = 0xe5;
	}
}
#endif

void diskstatus_CreateBlank(int row) {
	char fn2[13], fn[13];
	int n = 0;

	strcpy(fn, "DISK");

  strcpy(fn2, fn);
  strcat(fn2, DISK_EXTENSION);
  while (FileExists(fn2)) {
    MutateFilename(fn, n++);
    strcpy(fn2, fn);
    strcat(fn2, DISK_EXTENSION);
  }

	OSD_Puts("Creating file: ");
	OSD_Puts(fn2);
	OSD_Puts("\n");

#ifdef WRITE_E5_TO_DISK
	int result = SaveFile(fn2, SaveBlankDiskCallback, DISK_BLOCKS*512);
#else
	int result = SaveFile(fn2, NULL, DISK_BLOCKS*512);
#endif
  if(result)
		Menu_Set(diskstatus);
	else
		Menu_Set(loadfailed);
}

static struct menu_entry diskstatus[]=
{
	{MENU_ENTRY_SUBMENU,"Disk menu.",MENU_ACTION(diskstatus)},
	{MENU_ENTRY_SUBMENU,"",MENU_ACTION(diskstatus)},
	{MENU_ENTRY_CALLBACK,"Insert disk 0",MENU_ACTION(diskstatus_LoadDisk0)},
	{MENU_ENTRY_TOGGLE, "Write protect disk 0",MENU_ACTION(6)},
#if NR_DISKS==2
  {MENU_ENTRY_CALLBACK,"Insert disk 1",MENU_ACTION(diskstatus_LoadDisk1)},
	{MENU_ENTRY_TOGGLE,"Write protect disk 1",MENU_ACTION(7)},
#endif
	{MENU_ENTRY_CALLBACK,"Create blank disk",MENU_ACTION(diskstatus_CreateBlank)},
	{MENU_ENTRY_SUBMENU,"",MENU_ACTION(diskstatus)},
	{MENU_ENTRY_SUBMENU,"Back",MENU_ACTION(&topmenu)},
	{MENU_ENTRY_NULL,0,0}
};

#ifdef LOAD_INITIAL_ROM
static int romboot_LoadRomPack(const char *filename) {
	HW_HOST(REG_HOST_CONTROL)=HOST_CONTROL_RESET|HOST_CONTROL_DIVERT_SDCARD;
	Delay();
	HW_HOST(REG_HOST_CONTROL)=HOST_CONTROL_DIVERT_SDCARD;

	LoadROM(filename);
	Menu_Set(topmenu);
	OSD_Show(0);
	return 0;
}

void mainmenu_LoadRomPack(int row) {
	FileSelector_SetLoadFunction(romboot_LoadRomPack);
	FileSelector_Show(row);
}
#endif

#ifdef DEBUG
void debugit(int row) {
  char str[10];
  intToStringNoPrefix(str, (*(unsigned int *)DEBUGSTATE));
  str[8] = '\n';
  str[9] = '\0';
  OSD_Puts(str);
}
#endif

////////////////////////////////////////////////////////////////////////////////
// TOP LEVEL MENU
#ifndef NO_MACHINE_MENU
void MachineMenu(int row);
#endif
struct menu_entry topmenu[]=
{
#ifdef DEBUG
  {MENU_ENTRY_CALLBACK,"Debug",MENU_ACTION(&debugit)},
#endif
#ifdef LOAD_INITIAL_ROM
	{MENU_ENTRY_CALLBACK,"Boot rompack         \x10",MENU_ACTION(&mainmenu_LoadRomPack)},
#endif
#ifdef RESET_MENU
  {MENU_ENTRY_CALLBACK,"Reset",MENU_ACTION(&Reset)},
#endif
#ifdef OPTION_MENU
	{MENU_ENTRY_SUBMENU, "Options              \x10",MENU_ACTION(optionsmenu)},
#endif
#ifdef SWITCHES_MENU
	{MENU_ENTRY_SUBMENU, "Switches             \x10",MENU_ACTION(switchesmenu)},
#endif
#ifndef SAMCOUPE
	{MENU_ENTRY_TOGGLE,  "Double OSD window",MENU_ACTION(5)},
	{MENU_ENTRY_TOGGLE,  "Advanced memmap",MENU_ACTION(8)},
#endif
#ifndef DISABLE_TAPE
	{MENU_ENTRY_SUBMENU, "Tape menu            \x10",MENU_ACTION(tapestatus)},
#endif
	{MENU_ENTRY_SUBMENU, "Disk menu            \x10",MENU_ACTION(diskstatus)},
#ifndef NO_MACHINE_MENU
	{MENU_ENTRY_CALLBACK, "Machine menu         \x10",MENU_ACTION(MachineMenu)},
#endif
	{MENU_ENTRY_CALLBACK,"Exit",MENU_ACTION(&Menu_Hide)},
	{MENU_ENTRY_NULL,0,0}
};

// An error message
static struct menu_entry loadfailed[]=
{
	{MENU_ENTRY_SUBMENU,"Loading failed",MENU_ACTION(loadfailed)},
	{MENU_ENTRY_SUBMENU,"OK",MENU_ACTION(&topmenu)},
	{MENU_ENTRY_NULL,0,0}
};

#ifdef USE_LOAD_ROM
// An error message
static struct menu_entry tapeloaded[]=
{
	{MENU_ENTRY_SUBMENU,"Loading succeeded",MENU_ACTION(tapeloaded)},
	{MENU_ENTRY_SUBMENU,"OK",MENU_ACTION(&topmenu)},
	{MENU_ENTRY_NULL,0,0}
};

// Entrypoint for load rom in menu - display message

int LoadROMMenu(const char *filename) {
  int result = LoadROM(filename);

  if(result)
		Menu_Set(tapeloaded);
	else
		Menu_Set(loadfailed);
  return result;
}
#endif

int main(int argc,char **argv)
{
	int i;
	int dipsw=0;
	int rom_initialised;
	
	rom_initialised = (*(unsigned int *)HOSTSTATE) & 1;

	
	InitInterrupts();
	ClearKeyboard();
#ifndef NO_MACHINE_MENU
	MachineInit();
#endif
	Menu_Init();
	FatInit();
	OSD_Init();
	
	// Put the host core in reset while we initialise...
	
	if (!rom_initialised)
		HW_HOST(REG_HOST_CONTROL)=HOST_CONTROL_RESET|HOST_CONTROL_DIVERT_SDCARD;

	PS2Init();
	EnableInterrupts();

	OSD_Clear();
	OSD_Show(1);	// Call this over a few frames to let the OSD figure out where to place the window.
	// for(i=0;i<4;++i)
	// {
	// 	PS2Wait();	// Wait for an interrupt - most likely VBlank, but could be PS/2 keyboard
	// 	OSD_Show(1);	// Call this over a few frames to let the OSD figure out where to place the window.
	// }

//	MENU_SLIDER_VALUE(&rgbmenu[0])=8;
//	MENU_SLIDER_VALUE(&rgbmenu[1])=8;
//	MENU_SLIDER_VALUE(&rgbmenu[2])=8;

	HW_HOST(REG_HOST_CONTROL)=HOST_CONTROL_DIVERT_SDCARD; // Release reset but take control of the SD card
	OSD_Puts("Initializing SD card\n");

	if(!FindDrive())
		return(0);

#ifdef LOAD_INITIAL_ROM
	if (!rom_initialised)
		LoadInitialRom();
#endif
	
	// setup tape interface
#ifndef DISABLE_TAPE
	tapestatus[1].label = tapeLoadSize;
#ifdef SAVE_BUFFER
	tapestatus[3].label = tapeSaveSize;
#endif
	TapeInit();
	TapeUpdateStatus();
#endif
	DiskInit();
#ifndef NO_MACHINE_MENU
	UartInit();
#endif
  
	Menu_Set(topmenu);

	// bring out of reset
	OSD_Show(0);
	HW_HOST(REG_HOST_CONTROL)=HOST_CONTROL_DIVERT_SDCARD;
	while(1)
	{
		struct menu_entry *m;
		int visible;
		HandlePS2RawCodes();
		visible=Menu_Run();
		DiskHandler();
#ifndef DISABLE_TAPE
		TapeLoadBlock();
#endif
		// UartLoop();

		HW_HOST(REG_HOST_SW)=MENU_TOGGLE_VALUES;	// Send the new values to the hardware.
		// dipsw=MENU_CYCLE_VALUE(&topmenu[1]);	// Take the value of the TestPattern cycle menu entry.
//		HW_HOST(REG_HOST_SCALERED)=MENU_SLIDER_VALUE(&rgbmenu[0]);

		// If the menu's visible, prevent keystrokes reaching the host core.
		HW_HOST(REG_HOST_CONTROL)=(visible ?
				HOST_CONTROL_DIVERT_KEYBOARD|HOST_CONTROL_DIVERT_SDCARD :
				HOST_CONTROL_DIVERT_SDCARD); // Maintain control of the SD card so the file selector can work.
																 // If the host needs SD card access then we would release the SD
																 // card here, and not attempt to load any further files.
	}
	return(0);
}
