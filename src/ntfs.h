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

#ifndef __NTFS_H__
#define __NTFS_H__

#include "usuals.h"
#include "stddef.h"
#include "drive.h"

#pragma pack(push, ntfs)
#pragma pack(1)


// WARNING Assumptions:
const uint32 kNTFS_RecordLen = 0x0400;


const char kNTFS_SysId[] = "NTFS    ";

struct NTFS_BootSector
{
	byte jmp[3];			// Jump to the boot loader routine 
	char sysId[8];			// System Id: "NTFS    " 
	uint16 bytePerSec;		// Bytes per sector 
	byte secPerClus;		// Sectors per cluster 
	byte padding[7];		// Unused 
	byte mediaDescriptor;	// Media descriptor (a) 
	byte padding2[2];		// Unused 
	uint16 secPerTrack;		// Sectors per track 
	uint16 numHeads;		// Number of heads 
	byte padding3[8];		// Unused 
	uint32 signature;		// Always 80 00 80 00 
	uint64 cSectors;		// Number of sectors in the volume 
	uint64 offMFT;			// LCN of VCN 0 of the $MFT 
	uint64 offMFTMirr;		// LCN of VCN 0 of the $MFTMirr 
	uint32 clusPerMFT;		// Clusters per MFT Record (b) 
	uint32 clusPerIndex;	// Clusters per Index Record 
	uint64 serialNum;		// Volume serial number 
};

const uint32 kNTFS_RecMagic = 'ELIF';
const uint32 kNTFS_RecEnd = 0xFFFFFFFF;
const uint32 kNTFS_RecHeaderLen = 0x30;

const uint16 kNTFS_RecFlagUse = 0x01;
const uint16 kNTFS_RecFlagDir = 0x02;

struct NTFS_RecordHeader
{
	uint32 magic;			// Magic number 'FILE' 
	uint16 offUpdSeq;		// Offset to the update sequence 
	uint16 cwUpdSeq;		// Size in words of Update Sequence Number & Array (S) 
	uint64 logSeqNum;		// $LogFile Sequence Number (LSN) 
	uint16 seqNum;			// Sequence number 
	uint16 cHardlinks;		// Hard link count 
	uint16 x_offUpdSeqArr;	// Offset to Update Sequence Array (DON'T THINK SO)
	uint16 flags;			// Flags 
	uint32 cbRecord;		// Real size of the FILE record 
	uint32 cbAllocated;		// Allocated size of the FILE record 
	uint64 refBaseRecord;	// File reference to the base FILE record 
	uint16 nextAttrId;		// Next Attribute Id 
	uint16 padding;			// (XP) Align to 4 byte boundary 
	uint32 recordNum;		// (XP) Number of this MFT Record 
};



const uint16 kNTFS_AttrCompressed = 0x0001;
const uint16 kNTFS_AttrEncrypted = 0x0002;
const uint16 kNTFS_AttrSparse = 0x0004;

struct NTFS_AttribHeader
{
	uint32 type;			// Attribute Type (e.g. 0x10, 0x60) 
	uint32 cbAttribute;		// Length (including this header) 
	byte bNonResident;		// Non-resident flag 
	byte cName;				// Name length 
	uint16 offName;			// Offset to the Attribute 
	uint16 flags;			// Flags 
	uint16 idAttribute;		// Attribute Id (a) 
};

struct NTFS_AttribResident
{
	NTFS_AttribHeader header;
	
	uint32 cbAttribData;	// Length of the Attribute 
	uint16 offAttribData;	// Offset to the Attribute 
	byte bIndexed;			// Indexed flag 
	byte padding;			// 0x00 Padding 
};

struct NTFS_AttribNonResident
{
	NTFS_AttribHeader header;
	
	uint64 startVCN;		// Starting VCN 
	uint64 lastVCN;			// Last VCN 
	uint16 offDataRuns;		// Offset to the Data Runs 
	uint16 compUnitSize;	// Compression Unit Size (b) 
	uint32 padding;			// Padding 
	uint64 cbAllocated;		// Allocated size of the attribute (c) 
	uint64 cbAttribData;	// Real size of the attribute 
	uint64 cbInitData;		// Initialized data size of the stream (d) 
};


const uint32 kNTFS_ATTRIBUTE_LIST = 0x20;
const uint32 kNTFS_FILENAME = 0x30;
const uint32 kNTFS_DATA = 0x80;


const uint32 kNTFS_FileReadOnly = 0x0001;
const uint32 kNTFS_FileHidden = 0x0002;
const uint32 kNTFS_FileSystem = 0x0004;
const uint32 kNTFS_FileArchive = 0x0020;
const uint32 kNTFS_FileDevice = 0x0040;
const uint32 kNTFS_FileNormal = 0x0080;
const uint32 kNTFS_FileTemorary = 0x0100;
const uint32 kNTFS_FileSparse = 0x0200;
const uint32 kNTFS_FileReparse = 0x0400;
const uint32 kNTFS_FileCompressed = 0x0800;
const uint32 kNTFS_FileOffline = 0x1000;
const uint32 kNTFS_FileNotIndexed = 0x2000;
const uint32 kNTFS_FileEncrypted = 0x4000;

const byte kNTFS_NameSpacePOSIX = 0x00;
const byte kNTFS_NameSpaceWIN32 = 0x01;
const byte kNTFS_NameSpaceDOS = 0x02;
const byte kNTFS_NameSpaceWINDOS = 0x03;

const wchar_t kNTFS_MFTName[] = L"$MFT";

struct NTFS_AttrFileName
{
	uint64 refParent;		// File reference to the parent directory. 
	uint64 timeCreated;		// C Time - File Creation 
	uint64 timeAltered;		// A Time - File Altered 
	uint64 timeModified;	// M Time - MFT Changed 
	uint64 timeRead;		// R Time - File Read 
	uint64 cbAllocated;		// Allocated size of the file 
	uint64 cbFileSize;		// Real size of the file 
	uint32 flags;			// Flags, e.g. Directory, compressed, hidden 
	uint32 eaReparse;		// Used by EAs and Reparse 
	byte cFileName;			// Filename length in characters (L) 
	byte nameSpace;			// Filename namespace 
							// File Name comes here
};

struct NTFS_AttrListRecord
{
	uint32 type;		// Type 
	uint16 cbRecord;	// Record length 
	byte cName;			// Name length (N) 
	byte offName;		// Offset to Name (a) 
	uint64 startVCN;	// Starting VCN (b) 
	uint64 refAttrib;	// Base File Reference of the attribute 
	uint16 idAttribute;	// Attribute Id (c) 
						// Attribute name here
};

#pragma pack(pop, ntfs)

NTFS_AttribHeader* NTFS_FindAttribute(NTFS_RecordHeader* pRecord, uint32 attrType, void* pEnd);
NTFS_AttribHeader* NTFS_NextAttribute(NTFS_AttribHeader* pAttrib, uint32 attrType, void* pEnd);
 
void* NTFS_GetAttributeData(NTFS_AttribResident* pAttrib, void* pEnd);
bool NTFS_IsBetterNameSpace(byte n1, byte n2);
bool NTFS_DoFixups(byte* pCluster, uint32 cbCluster);


#define NTFS_RefToSector(info, ref) (((ref & 0xFFFFFFFFFFFF) * (kNTFS_RecordLen / kSectorSize)) + (info).firstSector + (info).offMFT)
#define NTFS_MakeFileTime(i64, ft) ((ft).dwLowDateTime = LOWDWORD(i64), (ft).dwHighDateTime = HIGHDWORD(i64))



//
// The record, attribute or whatever was invalid on disk
//
#define ERROR_NTFS_INVALID			10801L
#define ERROR_NTFS_NOTIMPLEMENT		10802L

#endif //__NTFS_H__