/* 
 * AUTHOR
 * Stef Walter
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
 * Send bug reports to: <stef@memberwebs.com>
 */

#include "usuals.h"
#include "ntfs.h"

#include "malloc.h"
#include "string.h"


ntfs_attribheader* ntfs_searchattribute(byte* location, uint32 attrType, byte* end, bool skip)
{
	/* Now we should be at attributes */
	while((!end || (location + sizeof(ntfs_attribheader) < end)) && 
		  *((uint32*)location) != kNTFS_RecEnd)
	{
		ntfs_attribheader* attrib = (ntfs_attribheader*)location;

		if(!skip)
		{
			if(attrib->type == attrType)
				return attrib;
		}
		else
			skip = false;

		location += attrib->cbAttribute;
	}

	return NULL;
}

byte* ntfs_getattributeheaders(ntfs_recordheader* record)
{
  byte* location = (byte*)record;
  ASSERT(record->offAttrs != 0);
  ASSERT(record->offAttrs < 0x100);
  location += record->offAttrs;
  return location;
}

ntfs_attribheader* ntfs_findattribute(ntfs_recordheader* record, uint32 attrType, byte* end)
{
	byte* location = ntfs_getattributeheaders(record);
	return ntfs_searchattribute(location, attrType, end, false);
}

ntfs_attribheader* ntfs_nextattribute(ntfs_attribheader* attrib, uint32 attrType, byte* end)
{
	return ntfs_searchattribute((byte*)attrib, attrType, end, true);
}

byte* ntfs_getattributedata(ntfs_attribresident* attrib, byte* end)
{
	byte* data = ((byte*)attrib) + attrib->offAttribData;
	if(data > end)
		return NULL;

	return data;
}

bool ntfs_isbetternamespace(byte n1, byte n2)
{
  /*
   * We like our namespaces in this order
	 * 1. WIN32
	 * 2. WIN32/DOS
	 * 3. DOS
	 * 4. POSIX
   */

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

bool ntfs_dofixups(byte* cluster, uint32 size)
{
	ntfs_recordheader* record = (ntfs_recordheader*)cluster;
	byte numSectors;
    uint16* updSeq;
    uint16* sectorFooter;
    byte i;

	ASSERT(size % kSectorSize == 0);
    numSectors = (byte)(size / kSectorSize);

    /* Check the number of sectors against array */
	if(record->cwUpdSeq - 1 < numSectors)
		numSectors = record->cwUpdSeq - 1;
      
	updSeq = (uint16*)(cluster + record->offUpdSeq);

	for(i = 0; i < numSectors; i++)
	{
        /*
		 * Check last 2 bytes in each sector against
		 * first double byte value in update sequence 
        */
		sectorFooter = (uint16*)((cluster + (kSectorSize - 2)) + (i * kSectorSize));
		if(*sectorFooter == updSeq[0])
			*sectorFooter = updSeq[i + 1];
		else
			return false;
	}

	return true;
}
