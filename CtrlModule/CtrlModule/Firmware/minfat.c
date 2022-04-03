/*
Copyright 2005, 2006, 2007 Dennis van Weeren
Copyright 2008, 2009 Jakub Bednarski

This file is part of Minimig

Minimig is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Minimig is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

This is a simple FAT16 handler. It works on a sector basis to allow fastest acces on disk
images.

11-12-2005 - first version, ported from FAT1618.C

JB:
2008-10-11  - added SeekFile() and cluster_mask
            - limited file create and write support added
2009-05-01  - modified LoadDirectory() and GetDirEntry() to support sub-directories (with limitation of 511 files/subdirs per directory)
            - added GetFATLink() function
            - code cleanup
2009-05-03  - modified sorting algorithm in LoadDirectory() to display sub-directories above files
2009-08-23  - modified ScanDirectory() to support page scrolling and parent dir selection
2009-11-22  - modified FileSeek()
            - added FileReadEx()
2009-12-15  - all entries are now sorted by name with extension
            - directory short names are displayed with extensions

2012-07-24  - Major changes to fit the MiniSOC project - AMR
*/

// #include <stdio.h>
// #include <string.h>
//#include <ctype.h>
#include "misc.h"

#include "spi.h"

#include "minfat.h"
#include "swap.h"
#include "osd.h"

const static char NOEXT[] = "   ";
const static char DOT[] = ".       ";
const static char DOTDOT[] = "..      ";

// Stubs to replace standard C library functions
#define puts OSD_Puts

static unsigned int directory_cluster;       // first cluster of directory (0 if root)
static unsigned int entries_per_cluster;     // number of directory entries per cluster

// internal global variables
unsigned int fat32;                // volume format is FAT32
static unsigned long fat_start;                // start LBA of first FAT table
static unsigned long data_start;               // start LBA of data field
static unsigned long root_directory_cluster;   // root directory cluster (used in FAT32)
static unsigned long root_directory_start;     // start LBA of directory table
static unsigned long root_directory_size;      // size of directory region in sectors
static unsigned int fat_number;               // number of FAT tables
unsigned int cluster_size;             // size of a cluster in sectors
unsigned long cluster_mask;             // binary mask of cluster number
unsigned long cluster_size_p2;				// power of 2
static unsigned long fat_size;                 // size of fat
static unsigned short fs_info_sector;
static unsigned long first_free_cluster = 2;
static unsigned long boot_sector;              // partition boot sector

static unsigned int current_directory_cluster;
static unsigned int current_directory_start;

static int partitioncount;

unsigned int dir_entries;             // number of entry's in directory table

unsigned char sector_buffer[512];       // sector buffer
//unsigned long fat_lba = 0xffffffff;
unsigned char fat_sector_buffer[512];       // sector buffer

#ifndef DISABLE_LONG_FILENAMES
char longfilename[260];
int prevlfn=0;
#endif

void FatInit() {
	directory_cluster = 0;
	entries_per_cluster = 0;

	fat32 = 0;
	fat_start = 0;
	data_start = 0;
	root_directory_cluster = 0;
	root_directory_start = 0;
	root_directory_size = 0;
	fat_number = 0;
	cluster_size = 0;
	cluster_mask = 0;
	cluster_size_p2 = 0;
	fat_size = 0;
	fs_info_sector = 0;
	first_free_cluster = 2;
	boot_sector = 0;

	current_directory_cluster = 0;
	current_directory_start = 0;

	partitioncount = 0;

	dir_entries = 0;

	#ifndef DISABLE_LONG_FILENAMES
	prevlfn=0;
	#endif
}

// forward declarations
static int IsLastCluster(unsigned long cluster);
static void GetFSinfo(void);

#define fat_buffer (*(FATBUFFER*)&fat_sector_buffer) // Don't need a separate buffer for this.

static int compare(const char *s1, const char *s2,int b)
{
	int i;
	for(i=0;i<b;++i)
	{
		if(*s1++!=*s2++)
			return(1);
	}
	return(0);
}

// FindDrive() checks if a card is present and contains FAT formatted primary partition
int FindDrive(void) {
	int i;

	i=5; // retry 5 times to initialise and read MBR
	fat32=0;
	while(--i>0) {
		if(sd_init()) {
	    if(sd_read_sector(0, sector_buffer)) // read MBR
				i=-1;
		}
	}
	if(!i) {	// Did we escape the loop?
		OSD_Puts("Card init failed\n");
		return(0);
	}

	boot_sector=0;
	partitioncount=1;

	// If we can identify a filesystem on block 0 we don't look for partitions
  if (compare((const char*)&sector_buffer[0x36], "FAT16   ",8)==0) // check for FAT16
		partitioncount=0;
  if (compare((const char*)&sector_buffer[0x52], "FAT32   ",8)==0) // check for FAT32
		partitioncount=0;

//	printf("%d partitions found\n",partitioncount);

	if(partitioncount)
	{
		// We have at least one partition, parse the MBR.
		struct MasterBootRecord *mbr=(struct MasterBootRecord *)sector_buffer;

		boot_sector = mbr->Partition[0].startlba;
		if(mbr->Signature==0x55aa)
				boot_sector=SwapBBBB(mbr->Partition[0].startlba);
		else if(mbr->Signature!=0xaa55) {
			puts("No partition sig\n");
			return 0;
		}

		if (!sd_read_sector(boot_sector, sector_buffer)) // read discriptor
	    return 0;
	}

    if (compare(sector_buffer+0x52, "FAT32   ",8)==0) // check for FAT16
			fat32=1;
	else if (compare(sector_buffer+0x36, "FAT16   ",8)!=0) // check for FAT32
	{
        puts("Bad part\n");
		return(0);
	}

    if (sector_buffer[510] != 0x55 || sector_buffer[511] != 0xaa)  // check signature
        return(0);

    // check for near-jump or short-jump opcode
    if (sector_buffer[0] != 0xe9 && sector_buffer[0] != 0xeb)
        return(0);

    // check if blocksize is really 512 bytes
    if (sector_buffer[11] != 0x00 || sector_buffer[12] != 0x02)
        return(0);

    // get cluster_size
    cluster_size = sector_buffer[13];
		debug(("cluster_size = %d\n", cluster_size));

    // calculate cluster mask
    cluster_mask = cluster_size - 1;

		unsigned long m=1;
		cluster_size_p2 = 0;
		while (cluster_mask & m) {
			cluster_size_p2 ++;
			m <<= 1;
		}

    fat_start = boot_sector + sector_buffer[0x0E] + (sector_buffer[0x0F] << 8); // reserved sector count before FAT table (usually 32 for FAT32)
	fat_number = sector_buffer[0x10];

    if (fat32)
    {
        if (compare((const char*)&sector_buffer[0x52], "FAT32   ",8) != 0) // check file system type
            return(0);

        dir_entries = cluster_size << 4; // total number of dir entries (16 entries per sector)
        root_directory_size = cluster_size; // root directory size in sectors
        fat_size = sector_buffer[0x24] + (sector_buffer[0x25] << 8) + (sector_buffer[0x26] << 16) + (sector_buffer[0x27] << 24);
        data_start = fat_start + (fat_number * fat_size);
        root_directory_cluster = sector_buffer[0x2C] + (sector_buffer[0x2D] << 8) + (sector_buffer[0x2E] << 16) + ((sector_buffer[0x2F] & 0x0F) << 24);
				fs_info_sector = sector_buffer[0x30] | (sector_buffer[0x31] << 8);
        root_directory_start = (root_directory_cluster - 2) * cluster_size + data_start;
				first_free_cluster = 2;

				GetFSinfo(); // get first free cluster according to fsinfo sector
    }
    else
    {
        // calculate drive's parameters from bootsector, first up is size of directory
        dir_entries = sector_buffer[17] + (sector_buffer[18] << 8);
        root_directory_size = ((dir_entries << 5) + 511) >> 9;

        // calculate start of FAT,size of FAT and number of FAT's
        fat_size = sector_buffer[22] + (sector_buffer[23] << 8);

        // calculate start of directory
        root_directory_start = fat_start + (fat_number * fat_size);
        root_directory_cluster = 0; // unused

        // calculate start of data
        data_start = root_directory_start + root_directory_size;
    }
	ChangeDirectory(0);
    return(1);
}

/************************************************************************************/
/* FAT CLUSTER MANAGEMENT ROUTINES */
/************************************************************************************/
static int GetClusterTable(unsigned long cluster);
static unsigned long ClusterFat(unsigned long cluster);

int GetCluster(int cluster) {
	if (!GetClusterTable(cluster))
		return 0;
	return ClusterFat(cluster);
}

static unsigned long ClusterFat(unsigned long cluster) {
	if (fat32)
		return SwapBBBB(fat_buffer.fat32[cluster & 0x7f]);
	else
		return SwapBB(fat_buffer.fat16[cluster & 0xff]);
}

#define FAT_BUFFER_EMPTY				0x7fffffff
#define FAT_BUFFER_MASK					0x7fffffff
#define FAT_BUFFER_MODIFIED			0x80000000
static unsigned long currentFatLba = FAT_BUFFER_EMPTY;

static void CommitFat(void) {
	int i;
	if (currentFatLba & FAT_BUFFER_MODIFIED) { // is modified
		if ((currentFatLba & FAT_BUFFER_MASK) != FAT_BUFFER_EMPTY) {
			debug(("Writing lba %ld\n", fat_start + (currentFatLba & FAT_BUFFER_MASK)));
			for (i=0; i<fat_number; i++) {
				sd_write_sector(fat_start + (currentFatLba & FAT_BUFFER_MASK) + (i*fat_size), (unsigned char*)&fat_buffer);
			}
		}

		currentFatLba &= FAT_BUFFER_MASK;
	}
}

static int GetClusterTable(unsigned long cluster) {
	// requesting different cluster
	unsigned long fatLba = fat32 ? (cluster >> 7) : (cluster >> 8);

	if (fatLba != (currentFatLba & FAT_BUFFER_MASK)) {
		CommitFat();
		if (!sd_read_sector(fat_start + fatLba, (unsigned char*)&fat_buffer))
			return(0);
		currentFatLba = fatLba;
	}
	return 1;
}

static unsigned long GetFreeCluster(unsigned long start) {
	do {
		if (!GetClusterTable(start))
			return 0;
		if (!ClusterFat(start)) {
			break;
		}
		start ++;
	} while (1); //TODO: to end of disk
	return start;
}

static void BlankCluster(unsigned long cluster) {
	unsigned long sb = data_start + cluster_size * (cluster-2);
	int i;

	memset(sector_buffer, 0, sizeof sector_buffer);
	for (i=0; i < cluster_size; i++) {
		sd_write_sector(sb+i, sector_buffer);
	}
}

static int IsLastCluster(unsigned long cluster) {
	if (fat32)
		return ((cluster & 0x0FFFFFF8) == 0x0FFFFFF8);
	else
		return cluster == 0xFFFF;
}

static unsigned long GetLastCluster(unsigned long start) {
	do {
		if (!GetClusterTable(start))
			return 0;

		if (IsLastCluster(ClusterFat(start))) {
			break;
		}
		start = ClusterFat(start);
	} while (1); //TODO: to end of disk
	return start;
}

static void GetFSinfo(void) {
	if (fat32 && sd_read_sector(boot_sector + fs_info_sector, sector_buffer) &&
			!compare(&sector_buffer[508], "\x00\x00\x55\xaa", 4)) {
		first_free_cluster = sector_buffer[0x1E8] |
			(sector_buffer[0x1E9] << 8) |
			(sector_buffer[0x1EA] << 16) |
			(sector_buffer[0x1EB] << 24);
	}
}

static void UpdateFSinfo(long clusterUsage) {
	if (fat32 && sd_read_sector(boot_sector + fs_info_sector, sector_buffer) &&
			!compare(&sector_buffer[508], "\x00\x00\x55\xaa", 4)) {
		unsigned long clusterFree = sector_buffer[0x1E8] |
			(sector_buffer[0x1E9] << 8) |
			(sector_buffer[0x1EA] << 16) |
			(sector_buffer[0x1EB] << 24);
		clusterFree += clusterUsage;
		sector_buffer[0x1E8] = clusterFree & 0xff;
		sector_buffer[0x1E9] = (clusterFree >> 8) & 0xff;
		sector_buffer[0x1EA] = (clusterFree >> 16) & 0xff;
		sector_buffer[0x1EB] = (clusterFree >> 24) & 0xff;

		first_free_cluster = clusterFree;
		sd_write_sector(boot_sector + fs_info_sector, sector_buffer);
	}
}

static void LinkClusters(unsigned long currentCluster, unsigned long nextCluster) {
	debug(("LinkClusters: %08lX -> %08lX\n", currentCluster, nextCluster));

	if (!GetClusterTable(currentCluster))
		return;
	if (fat32) {
		fat_buffer.fat32[currentCluster & 0x7f] = SwapBBBB(nextCluster);
	} else {
		fat_buffer.fat16[currentCluster & 0xff] = SwapBB(nextCluster);
	}
	currentFatLba |= FAT_BUFFER_MODIFIED;
}

static int FreeSpaceFat(unsigned long cluster) {
	unsigned long nextCluster;
	int nrFreed = 0;

	// reset cluster table
	currentFatLba = FAT_BUFFER_EMPTY;
	while (!IsLastCluster(cluster)) {
		if (!GetClusterTable(cluster)) return 0;
		nextCluster = ClusterFat(cluster);
		LinkClusters(cluster, 0);
		nrFreed ++;
		cluster = nextCluster;
	}
	CommitFat();
	UpdateFSinfo(nrFreed);
	return nrFreed;
}

static unsigned long ReserveSpaceFat(unsigned long size, unsigned long start) {
	int nrClusters;
	unsigned long currentCluster = 0, startCluster;
	unsigned long nextCluster = 0;
	int clustersRemaining;

	// calculates nr of clusters
	nrClusters = ((size + (512*cluster_size-1)) >> 9) >> cluster_size_p2;
	clustersRemaining = nrClusters;

	// reset cluster table
	currentFatLba = FAT_BUFFER_EMPTY;
	startCluster = currentCluster = GetFreeCluster(first_free_cluster);
	while (clustersRemaining > 1) {
		nextCluster = GetFreeCluster(currentCluster+1);
		LinkClusters(currentCluster, nextCluster);
		currentCluster = nextCluster;
		clustersRemaining --;
	}
	LinkClusters(currentCluster, fat32 ? 0x0fffffff : 0xffff);
	if (start != 0) {
		LinkClusters(start, startCluster);
	}
	CommitFat();
	UpdateFSinfo(-nrClusters);
	return startCluster;
}

int IsFat32() {
	return(fat32);
}

/************************************************************************************/
/* DIRECTORY HELPER ROUTINES */
/************************************************************************************/

static int NextDirEntryIsValid(DIRENTRY *pEntry) {
	return pEntry->Name[0] != SLOT_EMPTY &&
		pEntry->Name[0] != SLOT_DELETED &&
		!(pEntry->Attributes & ATTR_VOLUME);
}

DIRENTRY *NextDirEntry(int prev) {
	DIRENTRY *pEntry = NextDirEntryEx(current_directory_start, prev);
	return NextDirEntryIsValid(pEntry) ? pEntry : (DIRENTRY *)0;
}

DIRENTRY *NextDirEntryEx(unsigned long current_directory_start, int prev)
{
    unsigned long  iDirectory = 0;       // only root directory is supported
    DIRENTRY      *pEntry = NULL;        // pointer to current entry in sector buffer
    unsigned long  iDirectorySector;     // current sector of directory entries table
    unsigned long  iEntry;               // entry index in directory cluster or FAT16 root directory

	// FIXME traverse clusters if necessary

    iDirectorySector = current_directory_start+(prev>>4);

	if ((prev & 0x0F) == 0) // first entry in sector, load the sector
	{
		sd_read_sector(iDirectorySector, sector_buffer); // root directory is linear
	}
	pEntry = (DIRENTRY*)sector_buffer;
	pEntry+=(prev&0xf);
	if (pEntry->Name[0] != SLOT_EMPTY && pEntry->Name[0] != SLOT_DELETED) // valid entry??
	{
		if (!(pEntry->Attributes & ATTR_VOLUME)) // not a volume
		{
			if(!prevlfn)
				longfilename[0]=0;
			prevlfn=0;
			// FIXME - should check the lfn checksum here.
		}
#ifndef DISABLE_LONG_FILENAMES
		else if (pEntry->Attributes == ATTR_LFN)	// Do we have a long filename entry?
		{
			unsigned char *p=&pEntry->Name[0];
			int seq=p[0];
			int offset=((seq&0x1f)-1)*13;
			char *o=&longfilename[offset];
			*o++=p[1];
			*o++=p[3];
			*o++=p[5];
			*o++=p[7];
			*o++=p[9];

			*o++=p[0xe];
			*o++=p[0x10];
			*o++=p[0x12];
			*o++=p[0x14];
			*o++=p[0x16];
			*o++=p[0x18];

			*o++=p[0x1c];
			*o++=p[0x1e];
			prevlfn++;
		}
#endif
	}
	return(pEntry);
}

/************************************************************************************/
/* HIGHER LEVEL FILE MANAGEMENT ROUTINES */
/************************************************************************************/

void FilenameNormalise(char *out, const char *in) {
	int i=0, j=0;

	while (in[i] && j < 11) {
		if (in[i] == '.') while(j<8) out[j++] = ' ';
		else out[j++] = toupper(in[i]);
		i++;
	}

	while(j<11) out[j++] = ' ';
	out[11] = '\0';
}

int FileOpen(fileTYPE *file, const char *name)
{
    DIRENTRY      *pEntry = NULL;        // pointer to current entry in sector buffer
    unsigned long  iDirectorySector;     // current sector of directory entries table
    unsigned long  iDirectoryCluster;    // start cluster of subdirectory or FAT32 root directory
    unsigned long  iEntry;               // entry index in directory cluster or FAT16 root directory
		int isClusterDir = current_directory_cluster == root_directory_cluster;

    iDirectoryCluster = current_directory_cluster;
    iDirectorySector = current_directory_start;
		//fat_lba = 0xffffffff;

    while (1)
    {
        for (iEntry = 0; iEntry < dir_entries; iEntry++)
        {
            if ((iEntry & 0x0F) == 0) // first entry in sector, load the sector
            {
                sd_read_sector(iDirectorySector++, sector_buffer); // root directory is linear
                pEntry = (DIRENTRY*)sector_buffer;
            }
            else
                pEntry++;


            if (pEntry->Name[0] != SLOT_EMPTY && pEntry->Name[0] != SLOT_DELETED) // valid entry??
            {
                if (!(pEntry->Attributes & (ATTR_VOLUME | ATTR_DIRECTORY))) // not a volume nor directory
                {
                    if (compare((const char*)pEntry->Name, name,11) == 0)
                    {
                        file->size = SwapBBBB(pEntry->FileSize); 		// for 68000
                        file->cluster = SwapBB(pEntry->StartCluster);
												file->cluster += (fat32 ? (SwapBB(pEntry->HighCluster) & 0x0FFF) << 16 : 0);
                        file->sector = 0;

                        return(1);
                    }
                }
            }
        }

        if (isClusterDir) { // subdirectory is a linked cluster chain
            iDirectoryCluster = GetCluster(iDirectoryCluster); // get next cluster in chain
            if (IsLastCluster(iDirectoryCluster)) // check if end of cluster chain
                 break; // no more clusters in chain

            iDirectorySector = data_start + cluster_size * (iDirectoryCluster - 2); // calculate first sector address of the new cluster
        }
        else
            break;

    }
    return(0);
}


int FileNextSector(fileTYPE *file) {
    // increment sector index
    file->sector++;

    // cluster's boundary crossed?
    if ((file->sector&cluster_mask) == 0)
			file->cluster=GetCluster(file->cluster);

    return(1);
}

unsigned long FileGetLba(fileTYPE *file) {
		unsigned long sb;
		sb = data_start;                         // start of data in partition
		sb += cluster_size * (file->cluster-2);  // cluster offset
		sb += file->sector & cluster_mask;      // sector offset in cluster
		return sb;
}

int FileRead(fileTYPE *file, unsigned char *pBuffer) {
    unsigned long sb = FileGetLba(file);
		return sd_read_sector(sb, pBuffer); // read sector from drive
}


#ifndef DISABLE_WRITE
int FileWrite(fileTYPE *file, unsigned char *pBuffer)
{
    unsigned long sb;

    sb = data_start;                         // start of data in partition
    sb += cluster_size * (file->cluster-2);  // cluster offset
    sb += file->sector & cluster_mask;      // sector offset in cluster

	return(sd_write_sector(sb, pBuffer)); // read sector from drive
}
#endif


/************************************************************************************/
/* DIRECTORY MANAGEMENT ROUTINES */
/************************************************************************************/
void ChangeDirectory(DIRENTRY *p)
{
	if(p)
	{
		current_directory_cluster = SwapBB(p->StartCluster);
		current_directory_cluster |= fat32 ? (SwapBB(p->HighCluster) & 0x0FFF) << 16 : 0;
	} else {
		current_directory_cluster = 0;
	}
	if(current_directory_cluster)
	{
    current_directory_start = data_start + cluster_size * (current_directory_cluster - 2);
		dir_entries = cluster_size << 4;
	}
	else
	{
		current_directory_cluster = root_directory_cluster;
		current_directory_start = root_directory_start;
		dir_entries = fat32 ?  cluster_size << 4 : root_directory_size << 4; // 16 entries per sector
	}
}

void DirEnumOther(unsigned long directoryCluster, unsigned long directorySector,
								void *pv, int (*cb)(void *, DIRENTRY *, char *, int, int), int all) {
	int i;
	int lastprevlfn = 0, offset = 0, updated = 0;
	int cluster_based = directoryCluster != root_directory_cluster || fat32;

	while(1) {
		for (i=0; i<dir_entries; i++) {
			DIRENTRY *p = NextDirEntryEx(directorySector, i);
			if (all || NextDirEntryIsValid(p)) {
#ifdef DEBUG
					debug(("filename %s de %p (%s)\n", d->fileName, de, lfn)); show(de);
#endif
				int result = cb(pv, p, longfilename, offset+i, lastprevlfn);
				if (result < 0) updated = 1;
				else if (result > 0) {
					if (updated) sd_write_sector(directorySector+(i>>4), sector_buffer);
					break;
				}
			}
			lastprevlfn = prevlfn;
			if ((i & 0xf) == 0xf && updated) {
				sd_write_sector(directorySector+(i>>4), sector_buffer);
				updated = 0;
			}
		}
		if (i < dir_entries) break;

		if (cluster_based) {
			directoryCluster = GetCluster(directoryCluster); // get next cluster in chain
			if (IsLastCluster(directoryCluster)) // check if end of cluster chain
				 break; // no more clusters in chain

			directorySector = data_start + cluster_size * (directoryCluster - 2); // calculate first sector address of the new cluster
			offset += dir_entries;
		} else break;
	}
}

void DirEnum(void *pv, int (*cb)(void *, DIRENTRY *, char *, int, int), int all) {
	DirEnumOther(current_directory_cluster, current_directory_start, pv, cb, all);
}

struct DirCd_Struct {
	int foundIt;
	char fileName[12];
	char *lfn;
	DIRENTRY *thisOne;
	int startEntries;
  int endEntries;
};

static int DirCd_Callback(void *pv, DIRENTRY *de, char *lfn, int index, int prevlfn) {
	struct DirCd_Struct *d = (struct DirCd_Struct *)pv;
	if (de && !compare(de->Name, d->fileName, 8) && !compare(de->Extension, &d->fileName[8], 3)) {
		d->foundIt = 1;
		d->thisOne = de;
		d->startEntries = index-prevlfn;
    d->endEntries = index;
		d->lfn = lfn;
		return 1;
	}
	return 0;
}

int DirCd(const char *dir) {
	struct DirCd_Struct d;
	d.foundIt = 0;
	FilenameNormalise(d.fileName, dir);
	DirEnum(&d, DirCd_Callback, 0);
	if (d.foundIt) {
		ChangeDirectory(d.thisOne);
	}
	return d.foundIt;
}

#ifdef INCLUDE_FILE_REMOVE
static int FileRemoveAction_cb(void *pv, DIRENTRY *de, char *lfn, int index, int prevlfn) {
  struct DirCd_Struct *d = (struct DirCd_Struct *)pv;

  if (index >= d->startEntries && index <= d->endEntries) {
    de->Name[0] = SLOT_DELETED;
    return -1;
  } else if (index > d->endEntries) {
    return 1;
  }
  return 0;
}

int FileRm(const char *dir) {
	struct DirCd_Struct d;
	unsigned long startCluster;
	FilenameNormalise(d.fileName, dir);
	d.foundIt = 0;
	DirEnum(&d, DirCd_Callback, 0);

	if (d.foundIt) {
		startCluster = (SwapBB(d.thisOne->HighCluster) << 16) | SwapBB(d.thisOne->StartCluster);
		DirEnum(&d, FileRemoveAction_cb, 1);
		FreeSpaceFat(startCluster);
		return 1;
	}
	return 0;
}
#endif // DISABLE_FILE_REMOVE

int FileExists(const char *dir) {
	return FileExistsEx(dir, NULL);
}

int FileExistsEx(const char *dir, unsigned long *cluster) {
	struct DirCd_Struct d;
	FilenameNormalise(d.fileName, dir);
	d.foundIt = 0;
	DirEnum(&d, DirCd_Callback, 0);
	if (cluster) *cluster = (SwapBB(d.thisOne->HighCluster) << 16) | SwapBB(d.thisOne->StartCluster);
	return d.foundIt;
}

struct FileCreateSurvey_Struct {
	int blankEntry;
	char fileName[12];
	unsigned long fileSize;
	unsigned long clusterStart;
	unsigned char attr;
	int done;
};

static int FileCreateSurvey_cb(void *pv, DIRENTRY *de, char *lfn, int index, int prevlfn) {
	struct FileCreateSurvey_Struct *d = (struct FileCreateSurvey_Struct *)pv;
	if (de->Name[0] == SLOT_DELETED || de->Name[0] == SLOT_EMPTY) {
		d->blankEntry = index;
		debug(("FileCreateSurvey_cb: write to entry nr %d\n", index));
		return 1;
	}
	return 0;
}

static int FileCreateAction_cb(void *pv, DIRENTRY *de, char *lfn, int index, int prevlfn) {
	struct FileCreateSurvey_Struct *d = (struct FileCreateSurvey_Struct *)pv;
	if (d->done) return 1;
	else if (de->Name[0] == SLOT_DELETED || de->Name[0] == SLOT_EMPTY) {
		memcpy(de->Name, d->fileName, 8);
		memcpy(de->Extension, &d->fileName[8], 3);
		de->Attributes = d->attr;
		de->LowerCase = 0x00;
		de->CreateHundredth = 0x00;
		de->CreateTime = de->ModifyTime = 0;
		de->CreateDate = de->ModifyDate = 0;
		de->AccessDate = 0;
		de->StartCluster = SwapBB(d->clusterStart & 0xffff);
		de->HighCluster = SwapBB(d->clusterStart >> 16);
		de->FileSize = SwapBBBB(d->fileSize);
		d->done = 1;
		return -1;
	}
	return 0;
}

static int FileCreateEx(const char *fn, unsigned long fileSize, unsigned char attr,
		unsigned long *startCluster) {
	struct FileCreateSurvey_Struct d;

	// file exists, then dont write
	if (FileExists(fn)) return 0;

	// look for blank or deleted entry
	debug(("FileCreateEx: looking for a blank entry"));
	d.blankEntry = -1;
	DirEnum(&d, FileCreateSurvey_cb, 1);

	// no more space in fat32 file system - add another cluster to directory
	if (d.blankEntry < 0 && fat32) {
		debug(("FileCreateEx: could not find a blank entry\n"));
		unsigned long lastDirCluster = GetLastCluster(current_directory_cluster);
		// debug(("lastDirCluster = %08lX\n", lastDirCluster));
		unsigned long newCluster = ReserveSpaceFat(1, lastDirCluster);
		BlankCluster(newCluster);
		DirEnum(&d, FileCreateSurvey_cb, 1);
		debug(("FileCreateEx: created item\n"));
	}

	debug(("FileCreateEx: created item\n"));
	if (d.blankEntry >= 0) {
		// if exists, then reserve space and write file record
		FilenameNormalise(d.fileName, fn);
		debug(("FileCreateEx: normalise\n"));
		d.fileSize = attr == ATTR_DIRECTORY ? 0 : fileSize;
		d.clusterStart = ReserveSpaceFat(fileSize, 0);
		d.done = 0;
		d.attr = attr;
		debug(("FileCreateEx: reserved space\n"));
		DirEnum(&d, FileCreateAction_cb, 1);
		debug(("FileCreateEx: added file entry\n"));
		if (startCluster) *startCluster = d.clusterStart;
		return d.done;
	}

	return 0;
}

int FileCreate(const char *fn, unsigned long fileSize) {
	return FileCreateEx(fn, fileSize, ATTR_ARCHIVE, NULL);
}

#ifdef INCLUDE_DIR_MAKE
int DirMkdir(const char *fn) {
	unsigned long cluster;
	DIRENTRY *de = (DIRENTRY *)sector_buffer;

	if (FileCreateEx(fn, 512*cluster_size, ATTR_DIRECTORY, &cluster)) {
		memset(sector_buffer, 0, sizeof sector_buffer);


		memset(de, 0, sizeof (DIRENTRY));
		de->Attributes = ATTR_DIRECTORY;
		de->StartCluster = SwapBB(cluster & 0xffff);
		de->HighCluster = SwapBB(cluster >> 16);
		memcpy(de->Name, DOT, 8);
		memcpy(de->Extension, NOEXT, 3);

		de ++;
		memset(de, 0, sizeof (DIRENTRY));
		de->Attributes = ATTR_DIRECTORY;
		if (current_directory_cluster != root_directory_cluster) {
			de->StartCluster = SwapBB(current_directory_cluster & 0xffff);
			de->HighCluster = SwapBB(current_directory_cluster >> 16);
		}
		memcpy(de->Name, DOTDOT, 8);
		memcpy(de->Extension, NOEXT, 3);

		unsigned long sb = data_start + cluster_size * (cluster-2);
		int i;
		for (i=0; i < cluster_size; i++) {
			sd_write_sector(sb+i, sector_buffer);
			memset(sector_buffer, 0, sizeof sector_buffer);
		}
		return 1;

	}
	return 0;
}
#endif // #ifdef INCLUDE_DIR_MAKE

#ifdef INCLUDE_DIR_REMOVE
struct DirRemoveSurvey_Struct {
	int foundIt;
};

static int DirRemoveSurvey_cb(void *pv, DIRENTRY *de, char *lfn, int index, int prevlfn) {
	struct DirRemoveSurvey_Struct *d = (struct DirRemoveSurvey_Struct *)pv;

	debug(("DirRemoveSurvey_cb: file %s index %d de->Name[0] %02X\n", lfn, index, de->Name[0]));

	if (de->Name[0] == SLOT_DELETED || de->Name[0] == SLOT_EMPTY ||
		(!compare(de->Extension, NOEXT, 3) && (!compare(de->Name, DOTDOT, 8) || !compare(de->Name, DOT, 8)))) {
		// pass over
		return 0;
	}

	// a file exists
	debug(("a file exists - cannot delete it!!\n"));
	d->foundIt = 1;
	return 1;
}

int DirRm(const char *dir) {
	unsigned long cluster;
	struct DirRemoveSurvey_Struct d;

	if (!FileExistsEx(dir, &cluster)) {
		return 0;
	}

	d.foundIt = 0;

	debug(("before direnum\n"));
	DirEnumOther(cluster, data_start + cluster_size * (cluster - 2), &d, DirRemoveSurvey_cb, 0);
	debug(("finished dir enum\n"));
	if (d.foundIt) {
		// cannot remove - it has files
		debug(("file cannot be removed - files exist\n"));
		return 0;
	}

	// otherwise delete as normal
	debug(("file CAN be removed - Continuing\n"));
	return FileRm(dir);
}
#endif // #ifdef INCLUDE_DIR_REMOVE

unsigned long MaxLba() {
	return (fat_size*128*cluster_size)+root_directory_start;
}
