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

#include "scrounge.h"
#include "memref.h"
#include "ntfs.h"
#include "ntfsx.h"

ntfsx_datarun* ntfsx_datarun_alloc(byte* mem, byte* datarun)
{
  ntfsx_datarun* dr = (ntfsx_datarun*)malloc(sizeof(ntfsx_datarun));
  if(dr)
  {
    ASSERT(datarun);
	  dr->_mem = (byte*)refadd(mem);
	  dr->_datarun = datarun;
    dr->_curpos = NULL;

    dr->cluster = 0;
    dr->length = 0;
    dr->sparse = false;
  }

  return dr;
}

void ntfsx_datarun_free(ntfsx_datarun* dr)
{
  if(dr->_mem)
  {
    refrelease(dr->_mem);
    dr->_mem = NULL;
  }

  free(dr);
}

bool ntfsx_datarun_first(ntfsx_datarun* dr)
{
	dr->_curpos = dr->_datarun;
	dr->cluster = 0;
  dr->length = 0;
	dr->sparse = false;
	return ntfsx_datarun_next(dr);
}

bool ntfsx_datarun_next(ntfsx_datarun* dr)
{
  byte length;
  byte roffset;
	int64 offset;

	ASSERT(dr->_curpos);

	if(!*(dr->_curpos))
		return false;

	length = *(dr->_curpos) & 0x0F;
	roffset = *(dr->_curpos) >> 4;

	/* ASSUMPTION: length and offset are less 64 bit numbers */
	if(length == 0 || length > 8 || roffset > 8)
		return false;
		
	ASSERT(length <= 8);
	ASSERT(roffset <= 8);

	(dr->_curpos)++;

	memset(&(dr->length), 0, sizeof(uint64));

	memcpy(&(dr->length), (dr->_curpos), length);
	(dr->_curpos) += length;


	/* Note that offset can be negative */
	if(*((dr->_curpos) + (roffset - 1)) & 0x80)
		memset(&offset, ~0, sizeof(int64));
	else
		memset(&offset, 0, sizeof(int64));

	memcpy(&offset, (dr->_curpos), roffset);
	(dr->_curpos) += roffset;

	if(offset == 0)
	{
		dr->sparse = true;
	}
	else
	{
		dr->sparse = false;
		dr->cluster += offset;
	}

	return true;
}






bool ntfsx_cluster_reserve(ntfsx_cluster* clus, partitioninfo* info)
{
  ntfsx_cluster_release(clus);
  clus->size = CLUSTER_SIZE(*info);

  ASSERT(clus->size != 0);
  clus->data = (byte*)refalloc(clus->size);
  if(!clus->data)
  {
    errno = ENOMEM;
    return false;
  }
  
  return true;
}

bool ntfsx_cluster_read(ntfsx_cluster* clus, partitioninfo* info, uint64 begSector, int dd)
{
  int64 pos;
  size_t sz;

  if(!clus->data)
  {
    if(!ntfsx_cluster_reserve(clus, info))
      return false;
  }

  pos = SECTOR_TO_BYTES(begSector);
  if(_lseeki64(dd, pos, SEEK_SET) == -1)
    return false;

  sz = read(dd, clus->data, clus->size);
  if(sz == -1)
    return false;

  if(sz != clus->size)
  {
    errno = ERANGE;
    return false;
  }

	return true;
}

void ntfsx_cluster_release(ntfsx_cluster* clus)
{
  if(clus->data)
    refrelease(clus->data);

  clus->data = NULL;
  clus->size = 0;
}




ntfsx_attribute* ntfsx_attribute_alloc(ntfsx_cluster* clus, ntfs_attribheader* header)
{
  ntfsx_attribute* attr = (ntfsx_attribute*)malloc(sizeof(ntfsx_attribute));
  if(attr)
  {
    attr->_header = header;
    attr->_mem = (byte*)refadd(clus->data);
    attr->_length = clus->size;
  }

  return attr;
}

void ntfsx_attribute_free(ntfsx_attribute* attr)
{
  if(attr->_mem)
  {
    refrelease(attr->_mem);
    attr->_mem = NULL;
  }

  free(attr);
}

ntfs_attribheader* ntfsx_attribute_header(ntfsx_attribute* attr)
{
  return attr->_header;
}

void* ntfsx_attribute_getresidentdata(ntfsx_attribute* attr)
{
	ntfs_attribresident* res = (ntfs_attribresident*)attr->_header;
	ASSERT(!attr->_header->bNonResident);
	return (byte*)(attr->_header) + res->offAttribData;
}

uint32 ntfsx_attribute_getresidentsize(ntfsx_attribute* attr)
{
	ntfs_attribresident* res = (ntfs_attribresident*)attr->_header;
	ASSERT(!attr->_header->bNonResident);
	return res->cbAttribData;
}

ntfsx_datarun* ntfsx_attribute_getdatarun(ntfsx_attribute* attr)
{
	ntfs_attribnonresident* nonres = (ntfs_attribnonresident*)attr->_header;
	ASSERT(attr->_header->bNonResident);
	return ntfsx_datarun_alloc(attr->_mem, (byte*)(attr->_header) + nonres->offDataRuns);
}

bool ntfsx_attribute_next(ntfsx_attribute* attr, uint32 attrType)
{
	ntfs_attribheader* header = ntfs_nextattribute(attr->_header, attrType, 
                                    attr->_mem + attr->_length);	
	if(header)
	{
		attr->_header = header;
		return true;
	}

	return false;
}


ntfsx_record* ntfsx_record_alloc(partitioninfo* info)
{
  ntfsx_record* rec = (ntfsx_record*)malloc(sizeof(ntfsx_record));
  if(rec)
  {
    rec->info = info;
    memset(&(rec->_clus), 0, sizeof(ntfsx_cluster));
  }

  return rec;
}

void ntfsx_record_free(ntfsx_record* record)
{
  ntfsx_cluster_release(&(record->_clus));
  free(record);
}

bool ntfsx_record_read(ntfsx_record* record, uint64 begSector, int dd)
{
  ntfs_recordheader* rechead;

  if(!ntfsx_cluster_read(&(record->_clus), record->info, begSector, dd))
    err(1, "couldn't read mft record from drive");

	/* Check and validate this record */
	rechead = ntfsx_record_header(record);
	if(rechead->magic != kNTFS_RecMagic || 
	   !ntfs_dofixups(record->_clus.data, record->_clus.size))
	{
    warnx("invalid mft record");
    ntfsx_cluster_release(&(record->_clus));
    return false;
	}

  return true;
}

ntfs_recordheader* ntfsx_record_header(ntfsx_record* record)
{
  return (ntfs_recordheader*)(record->_clus.data); 
}

ntfsx_attribute* ntfsx_record_findattribute(ntfsx_record* record, uint32 attrType, int dd)
{
	ntfsx_attribute* attr = NULL;
  ntfs_attribheader* attrhead;
  ntfs_attriblistrecord* atlr;
  ntfs_attribresident* resident;
  uint64 mftRecord;
  ntfsx_record* r2;

	/* Make sure we have a valid record */
	ASSERT(ntfsx_record_header(record));
	attrhead = ntfs_findattribute(ntfsx_record_header(record), 
                        attrType, (record->_clus.data) + (record->_clus.size));

	if(attrhead)
	{
		attr = ntfsx_attribute_alloc(&(record->_clus), attrhead);
	}
	else
	{
		/* Do attribute list thing here! */
		attrhead = ntfs_findattribute(ntfsx_record_header(record), kNTFS_ATTRIBUTE_LIST, 
                                      (record->_clus.data) + (record->_clus.size));

    /* For now we only support Resident Attribute lists */
		if(dd && attrhead && !attrhead->bNonResident && record->info->mftmap)
		{
			resident = (ntfs_attribresident*)attrhead;
			atlr = (ntfs_attriblistrecord*)((byte*)attrhead + resident->offAttribData);

			/* Go through AttrList records looking for this attribute */
			while((byte*)atlr < (byte*)attrhead + attrhead->cbAttribute)
			{
				/* Found it! */
				if(atlr->type == attrType)
				{
					/* Read in appropriate cluster */
          mftRecord = ntfsx_mftmap_sectorforindex(record->info->mftmap, atlr->refAttrib & 0xFFFFFFFFFFFF);

					r2 = ntfsx_record_alloc(record->info);
          if(!r2)
            return NULL;

          if(ntfsx_record_read(r2, mftRecord, dd))
            attr = ntfsx_record_findattribute(r2, attrType, dd);

          ntfsx_record_free(r2);

          if(attr)
            break;
				}

 				atlr = (ntfs_attriblistrecord*)((byte*)atlr + atlr->cbRecord);
			}
		}
	}

	return attr;
}




struct _ntfsx_mftmap_block
{
  uint64 firstSector;   /* relative to the entire drive */
  uint64 length;        /* length in MFT records */
};

void ntfsx_mftmap_init(ntfsx_mftmap* map, partitioninfo* info)
{
  map->info = info;
  map->_blocks = NULL;
  map->_count = 0;
}

void ntfsx_mftmap_destroy(ntfsx_mftmap* map)
{
  if(map->_blocks)
  {
    free(map->_blocks);
    map->_blocks = NULL;
    map->_count = 0;
  }
}

bool ntfsx_mftmap_load(ntfsx_mftmap* map, ntfsx_record* record, int dd)
{
  bool ret = true;
	ntfsx_attribute* attribdata = NULL;   /* Data Attribute */
	ntfsx_datarun* datarun = NULL;	      /* Data runs for nonresident data */

  {
    ntfs_attribheader* header;
    ntfs_attribnonresident* nonres;
    uint64 length;
    uint64 firstSector;
    uint32 allocated;

    /* TODO: Check here whether MFT has already been loaded */
  
	  /* Get the MFT's data */
	  attribdata = ntfsx_record_findattribute(record, kNTFS_DATA, dd);
	  if(!attribdata)
      RETWARNBX("invalid mft. no data attribute");

    header = ntfsx_attribute_header(attribdata);
    if(!header->bNonResident)
      RETWARNBX("invalid mft. data attribute non-resident");

		datarun = ntfsx_attribute_getdatarun(attribdata);
    if(!datarun)
      RETWARNBX("invalid mft. no data runs");

		nonres = (ntfs_attribnonresident*)header;
    
    if(map->_blocks)
    {
      free(map->_blocks);
      map->_blocks = NULL;
    }

    map->_count = 0;
    allocated = 0;

		/* Now loop through the data run */
		if(ntfsx_datarun_first(datarun))
		{
			do
			{
        if(datarun->sparse)
          RETWARNBX("invalid mft. sparse data runs");

        if(map->_count >= allocated)
        {
          allocated += 16;
          map->_blocks = (struct _ntfsx_mftmap_block*)reallocf(map->_blocks, 
                  allocated * sizeof(struct _ntfsx_mftmap_block));
          if(!(map->_blocks))
      			errx(1, "out of memory");
        }

        ASSERT(map->info->cluster != 0);

        length = datarun->length * ((map->info->cluster * kSectorSize) / kNTFS_RecordLen);
        if(length == 0)
          continue;

        firstSector = (datarun->cluster * map->info->cluster) + map->info->first;
        if(firstSector >= map->info->end)
          continue;

        map->_blocks[map->_count].length = length;
        map->_blocks[map->_count].firstSector = firstSector;
        map->_count++;
			} 
			while(ntfsx_datarun_next(datarun));
    }

    ret = true;
  }

cleanup:

  if(attribdata)
    ntfsx_attribute_free(attribdata);
  if(datarun)
    ntfsx_datarun_free(datarun);

  return ret;
}

uint64 ntfsx_mftmap_length(ntfsx_mftmap* map)
{
  uint64 length = 0;
  uint32 i;

  for(i = 0; i < map->_count; i++)
    length += map->_blocks[i].length;

  return length;
}

uint64 ntfsx_mftmap_sectorforindex(ntfsx_mftmap* map, uint64 index)
{
  uint32 i;
  struct _ntfsx_mftmap_block* p;
  uint64 sector;

  for(i = 0; i < map->_count; i++)
  {
    p = map->_blocks + i;

    if(index >= p->length)
    {
      index -= p->length;
    }
    else
    {
      sector = index * (kNTFS_RecordLen / kSectorSize);
      sector += p->firstSector;

      if(sector >= map->info->end)
        return kInvalidSector;

      return sector;
    }
  }

  return kInvalidSector;
}
