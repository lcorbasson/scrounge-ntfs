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


#include "usuals.h"
#include "ntfs.h"

#include "malloc.h"
#include "string.h"


NTFS_AttribHeader* NTFS_SearchAttribute(byte* pLocation, uint32 attrType, void* pEnd, bool bSkip)
{
	// Now we should be at attributes
	while((!pEnd || (pLocation + sizeof(NTFS_AttribHeader) < pEnd)) && 
		  *((uint32*)pLocation) != kNTFS_RecEnd)
	{
		NTFS_AttribHeader* pAttrib = (NTFS_AttribHeader*)pLocation;

		if(!bSkip)
		{
			if(pAttrib->type == attrType)
				return pAttrib;
		}
		else
			bSkip = false;

		pLocation += pAttrib->cbAttribute;
	}

	return NULL;
}


NTFS_AttribHeader* NTFS_FindAttribute(NTFS_RecordHeader* pRecord, uint32 attrType, void* pEnd)
{
	byte* pLocation = (byte*)pRecord;
	pLocation += kNTFS_RecHeaderLen;

	return NTFS_SearchAttribute(pLocation, attrType, pEnd, false);
}

NTFS_AttribHeader* NTFS_NextAttribute(NTFS_AttribHeader* pAttrib, uint32 attrType, void* pEnd)
{
	return NTFS_SearchAttribute((byte*)pAttrib, attrType, pEnd, true);
}

void* NTFS_GetAttributeData(NTFS_AttribResident* pAttrib, void* pEnd)
{
	void* pData = ((byte*)pAttrib) + pAttrib->offAttribData;
	if(!pEnd && pData > pEnd)
		return NULL;

	return pData;
}

bool NTFS_IsBetterNameSpace(byte n1, byte n2)
{
	// We like our namespaces in this order
	// 1. WIN32
	// 2. WIN32/DOS
	// 3. DOS
	// 4. POSIX

	if(n1 == kNTFS_NameSpacePOSIX)
		return true;
	if(n1 == kNTFS_NameSpaceDOS &&
	   (n2 == kNTFS_NameSpaceWIN32 || n2 == kNTFS_NameSpaceWINDOS))
	    return true;
	if(n1 == kNTFS_NameSpaceWINDOS &&
	   n2 == kNTFS_NameSpaceWIN32)
	    return true;

	return false;
}

bool NTFS_DoFixups(byte* pCluster, uint32 cbCluster)
{
	ASSERT(cbCluster % kSectorSize == 0);
	NTFS_RecordHeader* pRecord = (NTFS_RecordHeader*)pCluster;

	byte numSectors = (byte)(cbCluster / kSectorSize);

	// Check the number of sectors against array
	if(pRecord->cwUpdSeq - 1 < numSectors)
		numSectors = pRecord->cwUpdSeq - 1;

	uint16* pUpdSeq = (uint16*)(pCluster + pRecord->offUpdSeq);

	for(byte i = 0; i < numSectors; i++)
	{
		// Check last 2 bytes in each sector against
		// first double byte value in update sequence 
		uint16* pSectorFooter = (uint16*)((pCluster + (kSectorSize - 2)) + (i * kSectorSize));
		if(*pSectorFooter == pUpdSeq[0])
			*pSectorFooter = pUpdSeq[i + 1];
		else
			return false;
	}

	return true;
}

