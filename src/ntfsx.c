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
  ntfsx_datarun* dr = (ntfsx_datarun*)mallocf(sizeof(ntfsx_datarun));

  ASSERT(datarun);
	dr->_mem = (byte*)refadd(mem);
	dr->_datarun = datarun;
  dr->_curpos = NULL;

  dr->cluster = 0;
  dr->length = 0;
  dr->sparse = false;

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






void ntfsx_cluster_reserve(ntfsx_cluster* clus, partitioninfo* info)
{
  ntfsx_cluster_release(clus);
  clus->size = CLUSTER_SIZE(*info);

  ASSERT(clus->size != 0);
  clus->data = (byte*)refalloc(clus->size);
}

bool ntfsx_cluster_read(ntfsx_cluster* clus, partitioninfo* info, uint64 begSector, int dd)
{
  int64 pos;
  size_t sz;

  if(!clus->data)
    ntfsx_cluster_reserve(clus, info);

  pos = SECTOR_TO_BYTES(begSector);
  if(lseek64(dd, pos, SEEK_SET) == -1)
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
  ntfsx_attribute* attr = (ntfsx_attribute*)mallocf(sizeof(ntfsx_attribute));
  attr->_header = header;
  attr->_mem = (byte*)refadd(clus->data);
  attr->_length = clus->size;
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



#define ATTR_ENUM_LISTPRI     1 << 1
#define ATTR_ENUM_DONEINLINE  1 << 2
#define ATTR_ENUM_DONELIST    1 << 3
#define ATTR_ENUM_FOUNDLIST   1 << 4

ntfsx_attrib_enum* ntfsx_attrib_enum_alloc(uint32 type, bool normal)
{
  ntfsx_attrib_enum* attrenum = (ntfsx_attrib_enum*)mallocf(sizeof(ntfsx_attrib_enum));
  attrenum->type = type;
  attrenum->_attrhead = NULL;
  attrenum->_listrec = NULL;
  attrenum->_flags = normal ? ATTR_ENUM_LISTPRI : 0;
  return attrenum;  
}

ntfsx_attribute* ntfsx_attrib_enum_inline(ntfsx_attrib_enum* attrenum, ntfsx_record* record)
{
  ntfsx_attribute* attr;
  ntfsx_cluster* cluster;
  ntfs_recordheader* rechead;

  /* If we're done */
  if(attrenum->_flags & ATTR_ENUM_DONEINLINE)
    return NULL;

  cluster = ntfsx_record_cluster(record);
  rechead = ntfsx_record_header(record);

  /* If this is the first time */
  if(!attrenum->_attrhead && !attrenum->_listrec)
  {
    attrenum->_attrhead = ntfs_findattribute(rechead, attrenum->type, 
                                              cluster->data + cluster->size);

    if(attrenum->_attrhead)
    {
      attr = ntfsx_attribute_alloc(cluster, attrenum->_attrhead);
      return attr;
    }

    /* Otherwise we fall through to the attr list stuff below */
  }

  /* Look for another attribute in the record */
  if(attrenum->_attrhead && attrenum->_attrhead->type == attrenum->type)
  {
    attrenum->_attrhead = ntfs_nextattribute(attrenum->_attrhead, attrenum->type,
                                          cluster->data + cluster->size);

    if(attrenum->_attrhead)
    {
      attr = ntfsx_attribute_alloc(cluster, attrenum->_attrhead);
      return attr;
    }

    /* Otherwise we're done */
  }

  attrenum->_flags |= ATTR_ENUM_DONEINLINE;
  return NULL;
}

ntfsx_attribute* ntfsx_attrib_enum_list(ntfsx_attrib_enum* attrenum, ntfsx_record* record)
{
  ntfsx_cluster* cluster;
  ntfs_recordheader* rechead;
  ntfs_attribresident* resident;
  ntfs_attribheader* attrhead;
  ntfsx_attribute* attr;
  uint64 mftRecord;
  ntfsx_record* r2;
  ntfsx_cluster* c2;

  ASSERT(record && attrenum);

  /* If we're done */
  if(attrenum->_flags & ATTR_ENUM_DONELIST)
    return NULL;
    
  cluster = ntfsx_record_cluster(record);
  rechead = ntfsx_record_header(record);

  /* Okay first check for attribute lists  */
  if(!attrenum->_listrec && !attrenum->_attrhead)
  {
    attrenum->_attrhead = ntfs_findattribute(rechead, kNTFS_ATTRIBUTE_LIST,
                                             cluster->data + cluster->size);
  
    /* If no attribute list, end of story */
    if(!attrenum->_attrhead)
    {
      attrenum->_flags |= ATTR_ENUM_DONELIST;
      return NULL;
    }

    /* We don't support non-resident attribute lists (which are stupid!) */
    if(attrenum->_attrhead->bNonResident)
    {
      warnx("brain dead, incredibly fragmented file data. skipping");
      attrenum->_flags |= ATTR_ENUM_DONELIST;
      return NULL;
    }

    /* We don't do attribute lists when no MFT loaded */
    if(!record->info->mftmap)
    {
      warnx("extended file attributes, but no MFT loaded. skipping");
      attrenum->_flags |= ATTR_ENUM_DONELIST;
      return NULL;
    }
  }

  /* This has to be set by now */
  ASSERT(attrenum->_attrhead);
  ASSERT(attrenum->_attrhead->type == kNTFS_ATTRIBUTE_LIST);

  resident = (ntfs_attribresident*)attrenum->_attrhead;

  for(;;)
  {
    if(attrenum->_listrec) /* progress to next record */
      attrenum->_listrec = (ntfs_attriblistrecord*)(((byte*)attrenum->_listrec) + attrenum->_listrec->cbRecord);

    else  /* get first record */
      attrenum->_listrec = (ntfs_attriblistrecord*)((byte*)resident + resident->offAttribData);

    if(((byte*)attrenum->_listrec) >= ((byte*)attrenum->_attrhead) + attrenum->_attrhead->cbAttribute)
    {
      attrenum->_listrec = NULL;
      attrenum->_flags |= ATTR_ENUM_DONELIST;
      return NULL;
    }

    if(attrenum->_listrec->type == attrenum->type)
    {
      attr = NULL;
      r2 = NULL;

		  /* Read in appropriate cluster */
      mftRecord = ntfsx_mftmap_sectorforindex(record->info->mftmap, attrenum->_listrec->refAttrib & kNTFS_RefMask);
      if(mftRecord == kInvalidSector)
      {
        warnx("invalid sector in mft map. screwed up file. skipping data");
      }
      else
      {
        r2 = ntfsx_record_alloc(record->info);

        if(ntfsx_record_read(r2, mftRecord, record->info->device))
        {
          rechead = ntfsx_record_header(r2);
          c2 = ntfsx_record_cluster(r2);
          attrhead = ntfs_findattribute(rechead, attrenum->type,
                                          c2->data + c2->size);

          if(attrhead)
            attr = ntfsx_attribute_alloc(c2, attrhead);
        }
      }

      if(r2)
        ntfsx_record_free(r2);

      if(attr)
        return attr;
    }
  }

  /* Not reached */
  ASSERT(false);
}

ntfsx_attribute* ntfsx_attrib_enum_all(ntfsx_attrib_enum* attrenum, ntfsx_record* record)
{
  ntfsx_attribute* attr = NULL;

  ASSERT(record && attrenum);

  /* 
   * When in this mode list attributes completely override
   * any inline attributes. This is the normal mode of 
   * operation
   */
  if(attrenum->_flags & ATTR_ENUM_LISTPRI)
  {
    if(!(attrenum->_flags & ATTR_ENUM_DONELIST))
    {
      attr = ntfsx_attrib_enum_list(attrenum, record);

      if(attr)
        attrenum->_flags |= ATTR_ENUM_FOUNDLIST;
    }

    if(!attr && !(attrenum->_flags & ATTR_ENUM_FOUNDLIST) && 
       !(attrenum->_flags & ATTR_ENUM_DONEINLINE))
      attr = ntfsx_attrib_enum_inline(attrenum, record);
  }

  /* 
   * The other mode of operation is to find everything 
   * inline first and then stuff in the lists.
   */
  else
  {
    if(!(attrenum->_flags & ATTR_ENUM_DONEINLINE))
      attr = ntfsx_attrib_enum_inline(attrenum, record);

    if(!attr && !(attrenum->_flags & ATTR_ENUM_DONELIST))
      attr = ntfsx_attrib_enum_list(attrenum, record);
  }

  return attr;
}  

void ntfsx_attrib_enum_free(ntfsx_attrib_enum* attrenum)
{
  free(attrenum);
} 



ntfsx_record* ntfsx_record_alloc(partitioninfo* info)
{
  ntfsx_record* rec = (ntfsx_record*)mallocf(sizeof(ntfsx_record));
  rec->info = info;
  memset(&(rec->_clus), 0, sizeof(ntfsx_cluster));
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
  {
    warn("couldn't read mft record from drive");
    return false;
  }

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

ntfsx_cluster* ntfsx_record_cluster(ntfsx_record* record)
{
  return &(record->_clus);
}

ntfs_recordheader* ntfsx_record_header(ntfsx_record* record)
{
  return (ntfs_recordheader*)(record->_clus.data); 
}

ntfsx_attribute* ntfsx_record_findattribute(ntfsx_record* record, uint32 attrType, int dd)
{
  ntfsx_attrib_enum* attrenum = NULL;
	ntfsx_attribute* attr = NULL;

  attrenum = ntfsx_attrib_enum_alloc(attrType, true);
  attr = ntfsx_attrib_enum_all(attrenum, record);
  ntfsx_attrib_enum_free(attrenum);
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

static void mftmap_expand(ntfsx_mftmap* map, uint32* allocated)
{
  if(map->_count >= *allocated)
  {
    (*allocated) += 16;
    map->_blocks = (struct _ntfsx_mftmap_block*)reallocf(map->_blocks, 
                  (*allocated) * sizeof(struct _ntfsx_mftmap_block));
  }
}

bool ntfsx_mftmap_load(ntfsx_mftmap* map, ntfsx_record* record, int dd)
{
  bool ret = true;
	ntfsx_attribute* attribdata = NULL;   /* Data Attribute */
	ntfsx_datarun* datarun = NULL;	      /* Data runs for nonresident data */
  ntfsx_attrib_enum* attrenum = NULL;

  {
    ntfs_attribheader* header;
    ntfs_attribnonresident* nonres;
    uint64 length;
    uint64 firstSector;
    uint32 allocated;
    uint64 total;
    bool hasdata = false;

    if(map->_blocks)
    {
      free(map->_blocks);
      map->_blocks = NULL;
    }

    map->_count = 0;
    allocated = 0;
    total = 0;
  
    attrenum = ntfsx_attrib_enum_alloc(kNTFS_DATA, false);

    while((attribdata = ntfsx_attrib_enum_all(attrenum, record)) != NULL)
    {
      header = ntfsx_attribute_header(attribdata);
      if(!header->bNonResident)
      {
        warnx("invalid mft. data attribute non-resident");
      }
      else
      {
  		  datarun = ntfsx_attribute_getdatarun(attribdata);
        if(!datarun)
        {
          warnx("invalid mft. no data runs in data attribute");
        }
        else
        {
          hasdata = true;
		      nonres = (ntfs_attribnonresident*)header;

          /* Check total length against nonres->cbAllocated */
          if(map->_count == 0)
            total = nonres->cbAllocated;

		      /* Now loop through the data run */
		      if(ntfsx_datarun_first(datarun))
		      {
			      do
			      {
              if(datarun->sparse)
              {
                warnx("invalid mft. sparse data runs");
              }
              else
              {
                mftmap_expand(map, &allocated);

                ASSERT(map->info->cluster != 0);

                length = datarun->length * ((map->info->cluster * kSectorSize) / kNTFS_RecordLen);
                if(length == 0)
                  continue;

                firstSector = (datarun->cluster * map->info->cluster) + map->info->first;
                if(firstSector >= map->info->end)
                  continue;

                /* 
                 * When the same as the last one skip. This occurs in really
                 * fragmented MFTs where we read the inline DATA attribute first
                 * and then move on to the ATTRLIST one.
                 */
                if(map->_count > 0 && map->_blocks[map->_count - 1].length == length &&
                   map->_blocks[map->_count - 1].firstSector == firstSector)
                  continue;

                map->_blocks[map->_count].length = length;
                map->_blocks[map->_count].firstSector = firstSector;
                map->_count++;

                total -= length * kSectorSize;
              }
			      } 
			      while(ntfsx_datarun_next(datarun));
          }

          ntfsx_datarun_free(datarun);
          datarun = NULL;
        }
      } 

      ntfsx_attribute_free(attribdata);
      attribdata = NULL;
    }

    if(!hasdata)
      RETWARNBX("invalid mft. no data attribute");

    ret = true;
  }

cleanup:

  if(attribdata)
    ntfsx_attribute_free(attribdata);
  if(datarun)
    ntfsx_datarun_free(datarun);
  if(attrenum)
    ntfsx_attrib_enum_free(attrenum);

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
