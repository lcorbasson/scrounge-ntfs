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

#ifndef __NTFS_H__
#define __NTFS_H__

#include "usuals.h"
#include "stddef.h"
#include "drive.h"
#include "compat.h"

typedef int16 ntfs_char;

#pragma pack(1)

/* WARNING Assumptions: */
#define kNTFS_RecordLen   0x0400

#define kNTFS_SysId       "NTFS    "

typedef struct ntfs_bootsector
{
	byte jmp[3];			    /* Jump to the boot loader routine */
	char sysId[8];			  /* System Id: "NTFS    " */
	uint16 bytePerSec;		/* Bytes per sector */
	byte secPerClus;		  /* Sectors per cluster */
	byte padding[7];		  /* Unused */
	byte mediaDescriptor;	/* Media descriptor (a) */
	byte padding2[2];		  /* Unused */
	uint16 secPerTrack;		/* Sectors per track */
	uint16 numHeads;		  /* Number of heads */
	byte padding3[8];		  /* Unused */
	uint32 signature;		  /* Always 80 00 80 00 */
	uint64 cSectors;		  /* Number of sectors in the volume */
	uint64 offMFT;			  /* LCN of VCN 0 of the $MFT */
	uint64 offMFTMirr;		/* LCN of VCN 0 of the $MFTMirr */
	uint32 clusPerMFT;		/* Clusters per MFT Record (b) */
	uint32 clusPerIndex;	/* Clusters per Index Record */
	uint64 serialNum;		  /* Volume serial number */
}
ntfs_bootsector;

#define kNTFS_RecMagic      ((uint32)'ELIF')
#define kNTFS_RecEnd        0xFFFFFFFF
#define kNTFS_RecHeaderLen  0x30

#define kNTFS_RecFlagUse    0x01
#define kNTFS_RecFlagDir    0x02

#ifdef _WIN32
  #define kNTFS_RefMask 0xFFFFFFFFFFFF
#else
  #define kNTFS_RefMask 0xFFFFFFFFFFFFLL
#endif

typedef struct _ntfs_recordheader
{
	uint32 magic;			    /* Magic number 'FILE' */
	uint16 offUpdSeq;		  /* Offset to the update sequence */
	uint16 cwUpdSeq;		  /* Size in words of Update Sequence Number & Array (S) */
	uint64 logSeqNum;		  /* $LogFile Sequence Number (LSN) */
	uint16 seqNum;			  /* Sequence number */
	uint16 cHardlinks;		/* Hard link count */
	uint16 offAttrs;	    /* Offset to Attributes */
	uint16 flags;			    /* Flags */
	uint32 cbRecord;		  /* Real size of the FILE record */
	uint32 cbAllocated;		/* Allocated size of the FILE record */
	uint64 refBaseRecord;	/* File reference to the base FILE record */
	uint16 nextAttrId;		/* Next Attribute Id */
	uint16 padding;			  /* (XP) Align to 4 byte boundary */
	uint32 recordNum;		  /* (XP) Number of this MFT Record */
}
ntfs_recordheader;


#define kNTFS_AttrCompressed    0x0001
#define kNTFS_AttrEncrypted     0x0002
#define kNTFS_AttrSparse        0x0004

typedef struct _ntfs_attribheader
{
	uint32 type;			    /* Attribute Type (e.g. 0x10, 0x60) */
	uint32 cbAttribute;		/* Length (including this header) */
	byte bNonResident;		/* Non-resident flag */
	byte cName;				    /* Name length */
	uint16 offName;			  /* Offset to the Attribute */
	uint16 flags;			    /* Flags */
	uint16 idAttribute;		/* Attribute Id (a) */
}
ntfs_attribheader;

typedef struct _ntfs_attribresident
{
	ntfs_attribheader header;
	
	uint32 cbAttribData;	/* Length of the Attribute */
	uint16 offAttribData;	/* Offset to the Attribute */
	byte bIndexed;			  /* Indexed flag */
	byte padding;			    /* 0x00 Padding */
}
ntfs_attribresident;

typedef struct _ntfs_attribnonresident
{
	ntfs_attribheader header;
	
	uint64 startVCN;		  /* Starting VCN */
	uint64 lastVCN;			  /* Last VCN */
	uint16 offDataRuns;		/* Offset to the Data Runs */
	uint16 compUnitSize;	/* Compression Unit Size (b) */
	uint32 padding;			  /* Padding */
	uint64 cbAllocated;		/* Allocated size of the attribute (c) */
	uint64 cbAttribData;	/* Real size of the attribute */
	uint64 cbInitData;		/* Initialized data size of the stream (d) */
}
ntfs_attribnonresident;


#define kNTFS_ATTRIBUTE_LIST    0x20
#define kNTFS_FILENAME          0x30
#define kNTFS_DATA              0x80


#define kNTFS_FileReadOnly      0x0001
#define kNTFS_FileHidden        0x0002
#define kNTFS_FileSystem        0x0004
#define kNTFS_FileArchive       0x0020
#define kNTFS_FileDevice        0x0040
#define kNTFS_FileNormal        0x0080
#define kNTFS_FileTemorary      0x0100
#define kNTFS_FileSparse        0x0200
#define kNTFS_FileReparse       0x0400
#define kNTFS_FileCompressed    0x0800
#define kNTFS_FileOffline       0x1000
#define kNTFS_FileNotIndexed    0x2000
#define kNTFS_FileEncrypted     0x4000

#define kNTFS_NameSpacePOSIX    0x00
#define kNTFS_NameSpaceWIN32    0x01
#define kNTFS_NameSpaceDOS      0x02
#define kNTFS_NameSpaceWINDOS   0x03

#ifdef FC_WIDE
#define kNTFS_MFTName           L"$MFT"
#else
#define kNTFS_MFTName           "$MFT"
#endif

typedef struct _ntfs_attribfilename
{
	uint64 refParent;		  /* File reference to the parent directory. */
	uint64 timeCreated;		/* C Time - File Creation */
	uint64 timeAltered;		/* A Time - File Altered */
	uint64 timeModified;	/* M Time - MFT Changed */
	uint64 timeRead;		  /* R Time - File Read */
	uint64 cbAllocated;		/* Allocated size of the file */
	uint64 cbFileSize;		/* Real size of the file */
	uint32 flags;			    /* Flags, e.g. Directory, compressed, hidden */
	uint32 eaReparse;		  /* Used by EAs and Reparse */
	byte cFileName;			  /* Filename length in characters (L) */
	byte nameSpace;		  	/* Filename namespace */
						          	/* File Name comes here */
}
ntfs_attribfilename;

typedef struct _ntfs_attriblistrecord
{
	uint32 type;		    /* Type */
	uint16 cbRecord;  	/* Record length */
	byte cName;			    /* Name length (N) */
	byte offName;		    /* Offset to Name (a)*/ 
	uint64 startVCN;	  /* Starting VCN (b) */
	uint64 refAttrib;	  /* Base File Reference of the attribute */
	uint16 idAttribute;	/* Attribute Id (c) */
						          /* Attribute name here */
}
ntfs_attriblistrecord;

#pragma pack()

ntfs_attribheader* ntfs_findattribute(ntfs_recordheader* record, uint32 attrType, byte* end);
ntfs_attribheader* ntfs_nextattribute(ntfs_attribheader* attrib, uint32 attrType, byte* end);
byte* ntfs_getattributelist(ntfs_recordheader* record);
byte* ntfs_getattributedata(ntfs_attribresident* attrib, byte* end);

bool ntfs_isbetternamespace(byte n1, byte n2);
bool ntfs_dofixups(byte* cluster, uint32 size);

/* TODO: Move these declarations elsewhere */
char* unicode_transcode16to8(const ntfs_char* src, size_t len);
ntfs_char* unicode_transcode8to16(const char* src, ntfs_char* out, size_t len);

#endif /* __NTFS_H__ */
