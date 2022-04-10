#include "misc.h"
#include "host.h"
#include "storage.h"
#include "minfat.h"

static fileTYPE file;

int GetBits(long filesize) {
	int c = filesize-1, bits = 0;

	while(c) {
		++bits;
		c>>=1;
	}
	return bits - 9;
}

#ifdef LOAD_INITIAL_ROM
int LoadROM(const char *filename)
{
	int result=0;
  char fnn[13];


	//HW_HOST(REG_HOST_CONTROL)=HOST_CONTROL_RESET; //Don't reset on load tape in ZX81 core
	HW_HOST(REG_HOST_CONTROL)=HOST_CONTROL_DIVERT_SDCARD; // Release reset but take control of the SD card
	HW_HOST(REG_HOST_BOOTDATATYPE) = TYPE_TAPE;
	
  FilenameNormalise(fnn, filename);
	if (FileOpen(&file,fnn))
	{
		int filesize=file.size;
		unsigned int c=0;
		int bits = GetBits(filesize);

		result=1;

		while(filesize>0)
		{
			OSD_ProgressBar(c,bits);
			if(FileRead(&file,sector_buffer))
			{
				int i;
				int *p=(int *)&sector_buffer;
				for(i=0;i<512;i+=4)
				{
					unsigned int t=*p++;
					HW_HOST(REG_HOST_BOOTDATA)=t;
				}
// 				intToStringNoPrefix(fnn, filesize);
// 				fnn[8] = '\n';
// 				fnn[9] = '\0';
// 				OSD_Puts(fnn);
			}
			else
			{
				result=0;
				filesize=512;
			}
			FileNextSector(&file);
			filesize-=512;
			++c;
		}
	}
	HW_HOST(REG_HOST_ROMSIZE) = file.size;

	return(result);
}
#endif

#ifdef USE_LOADFILE
int LoadFile(const char *filename, void (*callback)(unsigned char *data)) {
  fileTYPE file;
	int result=0;
	char fnn[13];

	FilenameNormalise(fnn, filename);

	if (FileOpen(&file, fnn)) {
		int filesize=file.size;
		unsigned int c=0;
		int bits = GetBits(filesize);

		result=1; // optimist
		while(filesize>0) {
			OSD_ProgressBar(c,bits);
			if(FileRead(&file,sector_buffer)) {
				callback(sector_buffer);
			} else {
        result = 0;
				break;
			}
			FileNextSector(&file);
			filesize-=512;
			++c;
		}
	} else {
		debug(("LoadFile: couldn't open file %s\n", filename));
	}
	return result ? file.size : 0;
}
#endif
int SaveFile(const char *fn, int (*callback)(unsigned char *data), long len) {
  fileTYPE file;
  int i;
  unsigned long data;
  long remaining = len;
	char fnn[13];

	// if file exists, delete
	debug(("SaveFile: Checking if file exists\n"));
	if (FileExists(fn)) {
		debug(("SaveFile: File exists remove it\n"));
#ifdef INCLUDE_FILE_REMOVE
    if (!FileRm(fn)) {
			debug(("SaveFile: failed to remove file\n"));
			return 0;
		}
#else
    return 0;
#endif
  }

	// now create the file
	debug(("SaveFile: creating file\n"));
	if (!FileCreate(fn, len)) {
		debug(("SaveFile: failed to create file\n"));
		return 0;
	}

	if (!callback) {
		// dont write to file, just create it
		return 1;
	}

	unsigned int c=0;
	int bits = GetBits(len);

	debug(("SaveFile: opening file to write\n"));
	FilenameNormalise(fnn, fn);
	debug(("SaveFile: normalised\n"));
  if (FileOpen(&file, fnn)) {
    debug(("found file\n"));
    while (remaining > 0) {
			OSD_ProgressBar(c,bits);
			if (callback(sector_buffer)) {
        FileWrite(&file, sector_buffer);
      }
      FileNextSector(&file);
      remaining -= 512;
			++c;
    }
    return 1;
  } else {
		debug(("Couldn't open file\n"));
	}

  return 0;
}


#ifdef UNDER_TEST
#undef HW_HOST
#define HW_HOST(r) HW_HOST_LL2(r)
#endif
int Save(const char *fn, long len) {
  fileTYPE file;
  int i;
  unsigned long data;
  long remaining = len;

  if (FileOpen(&file, fn)) {
    debug(("found file\n"));
    while (remaining > 0) {
      for (i=0; i<512; i+=4) {
        data = HW_HOST(REG_HOST_EXTDATA);
        sector_buffer[i+0] = (data >> 24) & 0xff;
        sector_buffer[i+1] = (data >> 16) & 0xff;
        sector_buffer[i+2] = (data >> 8) & 0xff;
        sector_buffer[i+3] = data & 0xff;
      }
      FileWrite(&file, sector_buffer);
      FileNextSector(&file);
      remaining -= 512;
    }
    return 1;
  }

  return 0;
}

int Open(const char *fn, void (*lbacb)(uint32_t)) {
  fileTYPE file;
  unsigned long lba;
  char fnn[13];

  FilenameNormalise(fnn, fn);

  if (FileOpen(&file, fnn)) {
    int filesize=file.size;
		unsigned int c=0;
		int bits = GetBits(filesize);

		while(filesize>0) {
			OSD_ProgressBar(c,bits);
      lba = FileGetLba(&file);

			lbacb(lba);

      debug(("lba = %ld\n", lba));
			FileNextSector(&file);
			filesize-=512;
			++c;
    }

    return file.size;
  }
  return 0;
}
