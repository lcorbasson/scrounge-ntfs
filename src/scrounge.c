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

#include "usuals.h"
#include "scrounge.h"
#include "ntfs.h"
#include "ntfsx.h"
#include "locks.h"

typedef struct _filebasics
{
  fchar_t filename[MAX_PATH + 1];
  uint64 created;
  uint64 modified;
  uint64 accessed;
  uint32 flags;
  uint64 parent;
}
filebasics;

void processRecordFileBasics(partitioninfo* pi, ntfsx_record* record, filebasics* basics)
{
  /* Data Attribute */
	ntfsx_attribute* attr = NULL; 

  {
    byte* resident = NULL;
    ntfs_attribfilename* filename;
    byte nameSpace;
    ntfs_char* name;
    size_t len;
#ifndef FC_WIDE
    char* temp;
#endif

    ASSERT(record);
    memset(basics, 0, sizeof(filebasics));
    basics->parent = kInvalidSector;

	  /* Now get the name and info... */
  	attr = ntfsx_record_findattribute(record, kNTFS_FILENAME, pi->device);
	  if(!attr) goto cleanup;

		nameSpace = kNTFS_NameSpacePOSIX;
		memset(basics->filename, 0, sizeof(basics->filename));

		do
		{
			/* TODO ASSUMPTION: File name is always resident */
			ASSERT(!ntfsx_attribute_header(attr)->bNonResident);


			/* Get out all the info we need */
			filename = (ntfs_attribfilename*)ntfsx_attribute_getresidentdata(attr);
      ASSERT(filename);
		
			/*
       * There can be multiple filenames with different 
       * namespaces so choose the best one
       */
			if(ntfs_isbetternamespace(nameSpace, filename->nameSpace))
			{
				/* Dates */
        basics->created = filename->timeCreated;
        basics->modified = filename->timeModified;
        basics->accessed = filename->timeRead;

				/* File Name */
        name = (ntfs_char*)(((byte*)filename) + sizeof(ntfs_attribfilename));
        len = filename->cFileName;
        if(len > MAX_PATH)
          len = MAX_PATH;

#ifdef FC_WIDE
        wcsncpy(basics->filename, name, len);
#else
        temp = unicode_transcode16to8(name, len);
        if(!temp)
          errx(1, "out of memory");

        len = strlen(temp);
        if(len > MAX_PATH)
          len = MAX_PATH;
        
        strncpy(basics->filename, temp, len);
#endif

        basics->filename[len] = 0;


				/* Attributes */
        basics->flags = filename->flags;


				/* Parent Directory */
        basics->parent = filename->refParent & kNTFS_RefMask;


				/* Namespace */
				nameSpace = filename->nameSpace;
			}
		}
		while(ntfsx_attribute_next(attr, kNTFS_FILENAME));
  }

cleanup:
  if(attr)
    ntfsx_attribute_free(attr);
}

void processMFTRecord(partitioninfo* pi, uint64 sector, int level)
{
  ntfsx_record* record = NULL;
  ntfsx_attribute* attribdata = NULL;
  ntfsx_datarun* datarun = NULL;
  int outfile = -1;

  ntfsx_cluster cluster;
  memset(&cluster, 0, sizeof(cluster));

  {
    filebasics basics;
    ntfs_recordheader* header;
    uint64 parentSector;
    uint64 dataSector;
    uint16 rename = 0;
    uint64 fileSize;
    uint32 i;
    uint32 num;
    fchar_t filename2[MAX_PATH + 1];
    ntfs_attribheader* attrhead;
    ntfs_attribnonresident* nonres;

    ASSERT(sector != kInvalidSector);

    record = ntfsx_record_alloc(pi);
    if(!record)
      errx(1, "out of memory");

    /* Read the MFT record */
	  if(!ntfsx_record_read(record, sector, pi->device))
      RETURN;

    header = ntfsx_record_header(record);
    ASSERT(header);

    if(!(header->flags & kNTFS_RecFlagUse))
      RETURN;

    /* Try and get a file name out of the header */
    processRecordFileBasics(pi, record, &basics);

    if(basics.filename[0] == 0)
    {
      RETWARNX("invalid mft record. in use, but no filename");
    }

    /* If it's the root folder then return */
    if(!fcscmp(basics.filename, FC_DOT))
      RETURN;

    /* Process parent folders if available */
    if(basics.parent != kInvalidSector)
    {
      /* Only if we have MFT map info available */
      if(pi->mftmap)
      {
        parentSector = ntfsx_mftmap_sectorforindex(pi->mftmap, basics.parent);

        if(parentSector == kInvalidSector)
          warnx("invalid parent directory for file: " FC_PRINTF, basics.filename);
        else
          processMFTRecord(pi, parentSector, level + 1);
      }
    }

    printf(level == 0 ? "\\" FC_PRINTF "\n" : "\\" FC_PRINTF, basics.filename);

    /* Directory handling: */
    if(header->flags & kNTFS_RecFlagDir)
    {
      /* Try to change to the directory */
      if(fc_chdir(basics.filename) == -1)
      {
        if(fc_mkdir(basics.filename) == -1)
        {
          warnx("couldn't create directory '" FC_PRINTF "' putting files in parent directory", basics.filename);
        }
        else
        {
          setFileAttributes(basics.filename, basics.flags);
          fc_chdir(basics.filename);
        }
      }

      RETURN;
    }


    /* Normal file handling: */
    outfile = fc_open(basics.filename, O_BINARY | O_CREAT | O_EXCL | O_WRONLY);
  
    fcsncpy(filename2, basics.filename, MAX_PATH);
    filename2[MAX_PATH] = 0;

    while(outfile == -1 && errno == EEXIST && rename < 0x1000)
    {
      if(fcslen(basics.filename) + 7 >= MAX_PATH)
      {
        warnx("file name too long on duplicate file: " FC_PRINTF, basics.filename);
        goto cleanup;
      }

      fcscpy(basics.filename, filename2);
      fcscat(basics.filename, FC_DOT);

      itofc(rename, basics.filename + fcslen(basics.filename), 10);
      rename++;

      outfile = fc_open(basics.filename, O_BINARY | O_CREAT | O_EXCL | O_WRONLY);
    }

    if(outfile == -1)
    {
      warnx("couldn't open output file: " FC_PRINTF, basics.filename);
      goto cleanup;
    }


    attribdata = ntfsx_record_findattribute(record, kNTFS_DATA, pi->device);
    if(!attribdata)
      RETWARNX("invalid mft record. no data attribute found");
  
    attrhead = ntfsx_attribute_header(attribdata);

    /* For resident data just write it out */
    if(!attrhead->bNonResident)
    {
      uint32 length = ntfsx_attribute_getresidentsize(attribdata);
      byte* data = ntfsx_attribute_getresidentdata(attribdata);

      if(!data)
        RETWARNX("invalid mft record. resident data screwed up");

      if(write(outfile, data, length) != (int32)length)
        RETWARN("couldn't write data to output file");
    }

    /* For non resident data it's a bit more involved */
    else
    {
      datarun = ntfsx_attribute_getdatarun(attribdata);
      if(!datarun)
        errx(1, "out of memory");

      nonres = (ntfs_attribnonresident*)attrhead;
      fileSize = nonres->cbAttribData;

      /* Allocate a cluster for reading and writing */
      if(!ntfsx_cluster_reserve(&cluster, pi))
        errx(1, "out of memory");

      if(ntfsx_datarun_first(datarun))
      {
        do
        {
          /* Check to see if we have a bogus data run */
          if(fileSize == 0 && datarun->length)
          {
            warnx("invalid mft record. file length invalid or extra data in file");
            break;
          }

          /* Sparse clusters we just write zeros */
          if(datarun->sparse)
          {
            memset(cluster.data, 0, cluster.size);

            for(i = 0; i < datarun->length && fileSize; i++)
            {
              num = cluster.size;

              if(fileSize < 0xFFFFFFFF && num > (uint32)fileSize)
                num = (uint32)fileSize;

              if(write(outfile, cluster.data, num) != (int32)num)
                err(1, "couldn't write to output file: " FC_PRINTF, basics.filename);

              fileSize -= num;
            }
          }

          /* Handle not sparse clusters */
          else
          {
            if(pi->locks)
            {
              /* Add a location lock so any raw scrounging won't do 
                 this cluster later */
              addLocationLock(pi->locks, CLUSTER_TO_SECTOR(*pi, datarun->cluster), 
                    CLUSTER_TO_SECTOR(*pi, datarun->cluster + datarun->length));
            }

            for(i = 0; i < datarun->length && fileSize; i++)
            {
              num = min(cluster.size, (uint32)fileSize);
              dataSector = CLUSTER_TO_SECTOR(*pi, (datarun->cluster + i));

              if(!ntfsx_cluster_read(&cluster, pi, dataSector, pi->device))
                err(1, "couldn't read sector from disk");

              if(write(outfile, cluster.data, num) != (int32)num)
                err(1, "couldn't write to output file: " FC_PRINTF, basics.filename);

              fileSize -= num;
            }
          }
        }
        while(ntfsx_datarun_next(datarun));
      }
    }

		if(fileSize != 0)
      warnx("invalid mft record. couldn't find all data for file");

    close(outfile);
    outfile = -1;

    setFileTime(basics.filename, &(basics.created), 
              &(basics.accessed), &(basics.modified));

    setFileAttributes(basics.filename, basics.flags);
  }

cleanup:
  if(record)
    ntfsx_record_free(record);

  ntfsx_cluster_release(&cluster);

  if(attribdata)
    ntfsx_attribute_free(attribdata);

  if(datarun)
    ntfsx_datarun_free(datarun);

  if(outfile != -1)
    close(outfile);
}


void scroungeMFT(partitioninfo* pi, ntfsx_mftmap* map)
{
  ntfsx_record* record = NULL;
  uint64 sector;
  filebasics basics;
  ntfs_recordheader* header;

  /* Try and find the MFT at the given location */
  sector = pi->mft + pi->first;

  if(sector >= pi->end)
    errx(2, "invalid mft. past end of partition");

  record = ntfsx_record_alloc(pi);
  if(!record)
    errx(1, "out of memory");

  /* Read the MFT record */
	if(!ntfsx_record_read(record, sector, pi->device))
		err(1, "couldn't read mft");

  header = ntfsx_record_header(record);
  ASSERT(header);

  if(!(header->flags & kNTFS_RecFlagUse))
    errx(2, "invalid mft. marked as not in use");

  /* Try and get a file name out of the header */

  processRecordFileBasics(pi, record, &basics);

  if(fcscmp(basics.filename, kNTFS_MFTName))
    errx(2, "invalid mft. wrong record");

  fprintf(stderr, "[Processing MFT...]\n");

  /* Load the MFT data runs */

  if(!ntfsx_mftmap_load(map, record, pi->device))
    err(1, "error reading in mft");

  if(ntfsx_mftmap_length(map) == 0)
    errx(1, "invalid mft. no records in mft");

  ntfsx_record_free(record);
}


void scroungeUsingMFT(partitioninfo* pi)
{
	uint64 numRecords = 0;
	fchar_t dir[MAX_PATH];
  ntfsx_mftmap map;
  uint64 length;
  uint64 sector;
  uint64 i;

  fprintf(stderr, "[Scrounging via MFT...]\n");

	/* Save current directory away */
	fc_getcwd(dir, MAX_PATH);

  /* Get the MFT map ready */
  memset(&map, 0, sizeof(map));
  ntfsx_mftmap_init(&map, pi);
  pi->mftmap = &map;


  /* 
   * Make sure the MFT is actually where they say it is.
   * This also fills in the valid cluster size if needed
   */
  scroungeMFT(pi, &map);
  length = ntfsx_mftmap_length(&map);
 
  for(i = 1; i < length; i ++)
  {
    sector = ntfsx_mftmap_sectorforindex(&map, i);
    if(sector == kInvalidSector)
    {
      warnx("invalid index in mft: %d", i);
      continue;
    }

    /* Process the record */
    processMFTRecord(pi, sector, 0);

  	/* Move to right output directory */
    fc_chdir(dir);
	}

  pi->mftmap = NULL;
}

void scroungeUsingRaw(partitioninfo* pi)
{
	byte buffSec[kSectorSize];
	fchar_t dir[MAX_PATH + 1];
  uint64 sec;
  drivelocks locks;
  int64 pos;
  size_t sz;
  uint32 magic = kNTFS_RecMagic;

  fprintf(stderr, "[Scrounging raw records...]\n");

	/* Save current directory away */
	fc_getcwd(dir, MAX_PATH);

  /* Get the locks ready */
  memset(&locks, 0, sizeof(locks));
  pi->locks = &locks;

	/* Loop through sectors */
	for(sec = pi->first; sec < pi->end; sec++)
	{
    if(checkLocationLock(&locks, sec))
      continue;

  	/* Read	the record */
    pos = SECTOR_TO_BYTES(sec);
    if(lseek64(pi->device, pos, SEEK_SET) == -1)
      errx(1, "can't seek device to sector");

    sz = read(pi->device, buffSec, kSectorSize);
    if(sz == -1 || sz != kSectorSize)
      errx(1, "can't read drive sector");

		/* Check beginning of sector for the magic signature */
		if(!memcmp(&magic, &buffSec, sizeof(magic)))
		{
      /* Process the record */
      processMFTRecord(pi, sec, 0);
		}

  	/* Move to right output directory */
    fc_chdir(dir);
	}

  pi->locks = NULL;
}
