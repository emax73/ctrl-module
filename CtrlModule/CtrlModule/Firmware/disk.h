#ifndef _DISK_H
#define _DISK_H

#ifdef SAMCOUPE
#	define DISK_EXTENSION      ".DSK"
#	define DISK_BLOCKS   1600
#else
#	define DISK_EXTENSION      ".OPD"
#	define DISK_BLOCKS   360
#endif
extern int DiskOpen(int i, const char *fn);
extern int DiskClose(void);
extern void DiskInit(void);
extern void DiskHandler(void);

#ifndef NR_DISKS
#define NR_DISKS    2
#endif

#endif
