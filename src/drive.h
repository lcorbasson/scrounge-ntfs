/* 
 * AUTHOR
 * N. Nielsen
 *
 * LICENSE
 * This software is in the public domain.
 *
 * The software is provided "as is", without warranty of any kind,
 * express or implied, including but not limited to the warranties
 * of merchantability, fitness for a particular purpose, and
 * noninfringement. In no event shall the author(s) be liable for any
 * claim, damages, or other liability, whether in an action of
 * contract, tort, or otherwise, arising from, out of, or in connection
 * with the software or the use or other dealings in the software.
 * 
 * SUPPORT
 * Send bug reports to: <nielsen@memberwebs.com>
 */

#ifndef __DRIVE_H__20010822
#define __DRIVE_H__20010822

#include "usuals.h"

#define kSectorSize     0x200

#ifdef _WIN32
  #define kInvalidSector  0xFFFFFFFFFFFFFFFF
#else
  #define kInvalidSector  0xFFFFFFFFFFFFFFFFLL
#endif


struct _ntfsx_mftmap;
struct _drivelocks;

typedef struct _partitioninfo
{
	uint32 first;		/* The first sector (in sectors) */
	uint32 end;		  /* The end sector (in sectors) */
	uint32 mft;			/* Offset into the MFT (in sectors) */
	byte cluster;		/* Cluster size (in sectors) */
  int device;     /* A handle to an open device */

  /* Some other context stuff about the drive */
  struct _drivelocks* locks;
  struct _ntfsx_mftmap* mftmap;
} 
partitioninfo;

#pragma pack(1)

#define kPartition_Invalid      0
#define kPartition_Extended     5
#define kPartition_ExtendedLBA 15


/* Partition table entry */
typedef struct _drive_partentry
{
	byte active;		    /* partition bootable flag */
	byte starthead;		  /* starting head */
	byte startsector;	  /* starting sector and 2 MS bits of cylinder */
	byte startcylinder;	/* starting cylinder (low 8 bits) */
	byte system;		    /* partition type */
	byte endhead;	    	/* ending head */
	byte endsector;	  	/* ending sector and 2 MS bits of cylinder */
	byte endcylinder; 	/* ending cylinder (low 8 bits) */
	uint32 startsec;	  /* absolute starting sector */
	uint32 endsec;		  /* absolute ending sector */
}
drive_partentry;

#define kMBR_Sig        0xAA55

/* Master Boot Record */
typedef struct _drive_mbr
{
	byte fill[0x1be];			          /* boot code */
	drive_partentry partitions[4];	/* partition table */
	uint16 sig;				              /* 55AAh boot signature */
}
drive_mbr;

#pragma pack()

#define CLUSTER_TO_SECTOR(info, clus) (((clus) * (info).cluster) + (info).first)
#define SECTOR_TO_BYTES(sec) ((sec) * kSectorSize)
#define CLUSTER_SIZE(info) ((info).cluster * kSectorSize)

#ifdef _WIN32
  /* driveName should be MAX_PATH chars long */
  void makeDriveName(char* driveName, int i);
#endif

#endif /* __DRIVE_H__ */
