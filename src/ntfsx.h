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

#ifndef __NTFSX_H__
#define __NTFSX_H__

#include "drive.h"
#include "ntfs.h"


/* used as a heap based object */
typedef struct _ntfsx_datarun
{
  byte* _mem; /* ref counted */
  byte* _datarun;
  byte* _curpos;

  bool sparse;
  uint64 cluster;
  uint64 length;
}
ntfsx_datarun;

ntfsx_datarun* ntfsx_datarun_alloc(byte* mem, byte* datarun);
void ntfsx_datarun_free(ntfsx_datarun* dr);
bool ntfsx_datarun_first(ntfsx_datarun* dr);
bool ntfsx_datarun_next(ntfsx_datarun* dr);



/* used as a stack based object */
typedef struct _ntfsx_cluster
{
	uint32 size;
	byte* data; /* ref counted */
}
ntfsx_cluster;

void ntfsx_cluster_reserve(ntfsx_cluster* clus, partitioninfo* info);
bool ntfsx_cluster_read(ntfsx_cluster* clus, partitioninfo* info, uint64 begSector, int dd);
void ntfsx_cluster_release(ntfsx_cluster* clus);
	


/* used as a heap based object */
typedef struct _ntfsx_attribute
{
	ntfs_attribheader* _header;
	byte* _mem;   /* ref counted */
	uint32 _length;
} 
ntfsx_attribute;

ntfsx_attribute* ntfsx_attribute_alloc(ntfsx_cluster* clus, ntfs_attribheader* header);
void ntfsx_attribute_free(ntfsx_attribute* attr);
ntfs_attribheader* ntfsx_attribute_header(ntfsx_attribute* attr);
void* ntfsx_attribute_getresidentdata(ntfsx_attribute* attr);
uint32 ntfsx_attribute_getresidentsize(ntfsx_attribute* attr);
ntfsx_datarun* ntfsx_attribute_getdatarun(ntfsx_attribute* attr);



/* used as a heap based object */
typedef struct _ntfsx_record
{
  partitioninfo* info;
  ntfsx_cluster _clus;
}
ntfsx_record;

ntfsx_record* ntfsx_record_alloc(partitioninfo* info);
ntfsx_cluster* ntfsx_record_cluster(ntfsx_record* record);
void ntfsx_record_free(ntfsx_record* record);
bool ntfsx_record_read(ntfsx_record* record, uint64 begSector, int dd);
bool ntfsx_record_validate(ntfsx_record* record);
ntfs_recordheader* ntfsx_record_header(ntfsx_record* record);
ntfsx_attribute* ntfsx_record_findattribute(ntfsx_record* record, uint32 attrType, int dd);


/* used as a heap based object */
typedef struct _ntfsx_attrib_enum
{
  ntfs_attribheader* _attrhead;     /* The last attribute examined */
  ntfs_attriblistrecord* _listrec;  /* The last attr list record examined */
  unsigned char _flags;             /* Whether to search through the list first */
  uint32 type;                      /* The type we're going for */
}
ntfsx_attrib_enum;

ntfsx_attrib_enum* ntfsx_attrib_enum_alloc(uint32 type, bool normal);
ntfsx_attribute* ntfsx_attrib_enum_all(ntfsx_attrib_enum* attrenum, ntfsx_record* record);
ntfsx_attribute* ntfsx_attrib_enum_inline(ntfsx_attrib_enum* attrenum, ntfsx_record* record);
ntfsx_attribute* ntfsx_attrib_enum_list(ntfsx_attrib_enum* attrenum, ntfsx_record* record);
void ntfsx_attrib_enum_free(ntfsx_attrib_enum* attrenum);



/* used as a stack based object */
struct _ntfsx_mftmap_block;
typedef struct _ntfsx_mftmap
{
  partitioninfo* info;
  struct _ntfsx_mftmap_block* _blocks;
  uint32 _count;
}
ntfsx_mftmap;

void ntfsx_mftmap_init(ntfsx_mftmap* map,partitioninfo* info);
void ntfsx_mftmap_destroy(ntfsx_mftmap* map);
bool ntfsx_mftmap_load(ntfsx_mftmap* map, ntfsx_record* record, int dd);
uint64 ntfsx_mftmap_length(ntfsx_mftmap* map);
uint64 ntfsx_mftmap_sectorforindex(ntfsx_mftmap* map, uint64 index);

#endif 
