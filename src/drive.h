// 
// AUTHOR
// N. Nielsen
//
// VERSION
// 0.7
// 
// LICENSE
// This software is in the public domain.
//
// The software is provided "as is", without warranty of any kind,
// express or implied, including but not limited to the warranties
// of merchantability, fitness for a particular purpose, and
// noninfringement. In no event shall the author(s) be liable for any
// claim, damages, or other liability, whether in an action of
// contract, tort, or otherwise, arising from, out of, or in connection
// with the software or the use or other dealings in the software.
// 
// SUPPORT
// Send bug reports to: <nielsen@memberwebs.com>
//


#ifndef __DRIVE_H__20010822
#define __DRIVE_H__20010822

#include "usuals.h"

const uint16 kSectorSize = 0x200;
struct DriveLock;

struct PartitionInfo
{
	uint32 firstSector;		
	uint32 lastSector;		
	uint32 offMFT;			// In sectors
	byte clusterSize;		// In sectors

	DriveLock* pLocks;
	uint32 cLocks;
	uint32 curLock;
};

PartitionInfo* CreatePartitionInfo();
void FreePartitionInfo(PartitionInfo* pInfo);


#pragma pack(push, drive)
#pragma pack(1)

const byte kPartition_Invalid = 0;
const byte kPartition_Extended = 5;
const byte kPartition_ExtendedLBA = 15;


// Partition table entry
struct Drive_PartEntry
{
	byte active;		// partition bootable flag
	byte starthead;		// starting head
	byte startsector;	// starting sector and 2 MS bits of cylinder
	byte startcylinder;	// starting cylinder (low 8 bits)
	byte system;		// partition type
	byte endhead;		// ending head
	byte endsector;		// ending sector and 2 MS bits of cylinder
	byte endcylinder;	// ending cylinder (low 8 bits)
	uint32 startsec;	// absolute starting sector
	uint32 endsec;		// absolute ending sector
};

const uint16 kMBR_Sig = 0xAA55;

// Master Boot Record
struct Drive_MBR
{
	byte fill[0x1be];			// boot code
	Drive_PartEntry partitions[4];	// partition table
	uint16 sig;				// 55AAh boot signature
};

#pragma pack(pop, drive)

#define CLUSTER_TO_SECTOR(info, clus) (((clus) * (info).clusterSize) + (info).firstSector)
#define SECTOR_TO_BYTES(sec) ((sec) * kSectorSize)
#define CLUSTER_SIZE(info) ((info).clusterSize * kSectorSize)

bool IntersectRange(uint64& b1, uint64& e1, uint64 b2, uint64 e2);
void AddLocationLock(PartitionInfo* pInfo, uint64 beg, uint64 end);
bool CheckLocationLock(PartitionInfo* pInfo, uint64& sec);



#endif //__DRIVE_H__20010822