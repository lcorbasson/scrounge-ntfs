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

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include "usuals.h"
#include "scrounge.h"
#include "ntfs.h"
#include "ntfsx.h"
#include "locks.h"

#define PROCESS_MFT_FLAG_SUB      1 << 1
#define DEF_FILE_MODE 0x180
#define DEF_DIR_MODE 0x1C0

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
  ntfsx_attrib_enum* attrenum = NULL;

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
  	attrenum = ntfsx_attrib_enum_alloc(kNTFS_FILENAME, true);

		nameSpace = kNTFS_NameSpacePOSIX;
		memset(basics->filename, 0, sizeof(basics->filename));

		while((attr = ntfsx_attrib_enum_all(attrenum, record)) != NULL)
		{
			/* TODO ASSUMPTION: File name is always resident */
			if(!ntfsx_attribute_header(attr)->bNonResident)
      {
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

      ntfsx_attribute_free(attr);
      attr = NULL;
		}
  }

  if(attr)
    ntfsx_attribute_free(attr);

  if(attrenum)
    ntfsx_attrib_enum_free(attrenum);
}

void processMFTRecord(partitioninfo* pi, uint64 sector, uint32 flags)
{
  ntfsx_record* record = NULL;
  ntfsx_attribute* attribdata = NULL;
  ntfsx_attrib_enum* attrenum = NULL;
  ntfsx_datarun* datarun = NULL;
  int ofile = -1;

  ntfsx_cluster cluster;
  memset(&cluster, 0, sizeof(cluster));

  {
    filebasics basics;
    ntfs_recordheader* header;
    uint64 parentSector;
    uint64 dataSector;
    uint16 rename = 0;
    uint64 dataSize = 0;       /* Length of initialized file data */
    uint64 sparseSize = 0;     /* Length of sparse data following */
    uint32 i;
    bool haddata = false;
    uint32 num;
    fchar_t filename2[MAX_PATH + 1];
    ntfs_attribheader* attrhead;
    ntfs_attribnonresident* nonres;

    ASSERT(sector != kInvalidSector);

    record = ntfsx_record_alloc(pi);

    /* Read the MFT record */
	  if(!ntfsx_record_read(record, sector, pi->device))
      RETURN;

    header = ntfsx_record_header(record);
    ASSERT(header);

    if(!(header->flags & kNTFS_RecFlagUse))
      RETURN;

    /* Try and get a file name out of the header */
    processRecordFileBasics(pi, record, &basics);

    /* Without files we skip */
    if(basics.filename[0] == 0)
      RETURN;

    /* If it's the root folder then return */
    if(!fcscmp(basics.filename, FC_DOT))
      RETURN;

#if 0
    printf("SECTOR: %llu", (unsigned long long)sector);
#endif
    printf(flags & PROCESS_MFT_FLAG_SUB ?
            "\\" FC_PRINTF : "\\" FC_PRINTF "\n", basics.filename);

    /* System, Hidden files that begin with $ are skipped */
    if(basics.flags & kNTFS_FileSystem && 
       basics.flags & kNTFS_FileHidden &&
       basics.filename[0] == kNTFS_SysPrefix)
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
          processMFTRecord(pi, parentSector, flags | PROCESS_MFT_FLAG_SUB);
      }
    }

    /* Directory handling: */
    if(header->flags & kNTFS_RecFlagDir)
    {
      /* Try to change to the directory */
      if(fc_chdir(basics.filename) == -1)
      {
#ifdef _DEBUG
        if(!g_verifyMode)
#endif
        {
#ifdef _WIN32
          if(fc_mkdir(basics.filename) == -1)
#else
		  if(fc_mkdir(basics.filename, DEF_DIR_MODE) == -1)
#endif
          {
            warn("couldn't create directory '" FC_PRINTF "' putting files in parent directory", basics.filename);
          }
          else
          {
            setFileAttributes(basics.filename, basics.flags);
            fc_chdir(basics.filename);
          }
        }
      }

      RETURN;
    }

#ifdef _DEBUG 
    /* If in verify mode */
    if(g_verifyMode)
    {
      ofile = fc_open(basics.filename, O_BINARY | O_RDONLY);

      if(ofile == -1)
      {
        warn("couldn't open verify file: " FC_PRINTF, basics.filename);
        goto cleanup;
      }
    }

    /* Normal file handling: */
    else
#endif
    {
      ofile = fc_open(basics.filename, O_BINARY | O_CREAT | O_EXCL | O_WRONLY, DEF_FILE_MODE);
  
      fcsncpy(filename2, basics.filename, MAX_PATH);
      filename2[MAX_PATH] = 0;

      while(ofile == -1 && errno == EEXIST && rename < 0x1000)
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

        ofile = fc_open(basics.filename, O_BINARY | O_CREAT | O_EXCL | O_WRONLY, DEF_FILE_MODE);
      }

      if(ofile == -1)
      {
        warn("couldn't open output file: " FC_PRINTF, basics.filename);
        goto cleanup;
      }
    }

    attrenum = ntfsx_attrib_enum_alloc(kNTFS_DATA, true);

    while((attribdata = ntfsx_attrib_enum_all(attrenum, record)) != NULL)
    {
      attrhead = ntfsx_attribute_header(attribdata);

      /* 
       * We don't do compressed/encrypted files. Eventually
       * we may be able to write in some support :)
       */
      if(attrhead->flags & kNTFS_AttrCompressed)
        RETWARNX("compressed file. skipping.");

      if(attrhead->flags & kNTFS_AttrEncrypted)
        RETWARNX("encrypted file. skipping.");

      /* On the first round figure out the file size */
      if(!haddata)
      {
        if(attrhead->bNonResident)
        {
          nonres = (ntfs_attribnonresident*)attrhead;

          if(nonres->cbInitData > nonres->cbAttribData)
            RETWARNX("invalid file length.");

          dataSize = nonres->cbInitData;
          sparseSize = nonres->cbAttribData - nonres->cbInitData;
        }
        else
        {
          dataSize = ntfsx_attribute_getresidentsize(attribdata);
          sparseSize = 0;
        }
      }

      haddata = true;

      /* For resident data just write it out */
      if(!attrhead->bNonResident)
      {
        uint32 length = ntfsx_attribute_getresidentsize(attribdata);
        byte* data = ntfsx_attribute_getresidentdata(attribdata);

        if(!data)
          RETWARNX("invalid mft record. resident data screwed up");

#ifdef _DEBUG
        if(g_verifyMode)
        {
          if(compareFileData(ofile, data, length) != 0)
            RETWARNX("verify failed. read file data wrong.");
        }
        else
#endif
          if(write(ofile, data, length) != (int32)length)
            RETWARN("couldn't write data to output file");

        dataSize -= length;
      }

      /* For non resident data it's a bit more involved */
      else
      {
        datarun = ntfsx_attribute_getdatarun(attribdata);
        nonres = (ntfs_attribnonresident*)attrhead;

        /* Allocate a cluster for reading and writing */
        ntfsx_cluster_reserve(&cluster, pi);

        if(ntfsx_datarun_first(datarun))
        {
          do
          {
            /* 
             * In some cases NTFS sloppily leaves many extra
             * data runs mapped for a file, so just cut out 
             * when that's the case 
             */
            if(dataSize == 0)
              break;

            /* Sparse clusters we just write zeros */
            if(datarun->sparse)
            {
              memset(cluster.data, 0, cluster.size);

              for(i = 0; i < datarun->length && dataSize; i++)
              {
                num = cluster.size;

                if(dataSize < 0xFFFFFFFF && num > (uint32)dataSize)
                  num = (uint32)dataSize;

#ifdef _DEBUG
                if(g_verifyMode)
                {
                  if(compareFileData(ofile, cluster.data, num) != 0)
                    RETWARNX("verify failed. read file data wrong.");
                }
                else
#endif
                  if(write(ofile, cluster.data, num) != (int32)num)
                    err(1, "couldn't write to output file: " FC_PRINTF, basics.filename);

                dataSize -= num;
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

              for(i = 0; i < datarun->length && dataSize; i++)
              {
                num = min(cluster.size, (uint32)dataSize);
                dataSector = CLUSTER_TO_SECTOR(*pi, (datarun->cluster + i));

                if(!ntfsx_cluster_read(&cluster, pi, dataSector, pi->device))
                {
                  warn("couldn't read sector from disk");
                  break;
                }

#ifdef _DEBUG
                if(g_verifyMode)
                {
                  if(compareFileData(ofile, cluster.data, num) != 0)
                    RETWARNX("verify failed. read file data wrong.");
                }
                else
#endif
                  if(write(ofile, cluster.data, num) != (int32)num)
                    err(1, "couldn't write to output file: " FC_PRINTF, basics.filename);

                dataSize -= num;
              }
            }
          }
          while(ntfsx_datarun_next(datarun));
        }

        ntfsx_datarun_free(datarun);
        datarun = NULL;
      }

      ntfsx_attribute_free(attribdata);
      attribdata = NULL;

      /* Cut out when there's extra clusters allocated */
      if(dataSize == 0)
        break;
    }

    if(!haddata)
      RETWARNX("invalid mft record. no data attribute found");

		if(dataSize != 0)
      warnx("invalid mft record. couldn't find all data for file");

    /* 
     * Now we write blanks for all the sparse non-inited data 
     * We might be able to just resize the file to the right 
     * size, but let's go the safe way and write out zeros
     */

    if(sparseSize > 0)
    {
      ntfsx_cluster_reserve(&cluster, pi);
      memset(cluster.data, 0, cluster.size);

      while(sparseSize > 0)
      {
        num = cluster.size;

        if(sparseSize < 0xFFFFFFFF && num > (uint32)sparseSize)
          num = (uint32)sparseSize;

#ifdef _DEBUG
        if(g_verifyMode)
        {
          if(compareFileData(ofile, cluster.data, num) != 0)
            RETWARNX("verify failed. read file data wrong.");
        }
        else
#endif
          if(write(ofile, cluster.data, num) != (int32)num)
            err(1, "couldn't write to output file: " FC_PRINTF, basics.filename);

        sparseSize -= num;
      }
    }

    close(ofile);
    ofile = -1;

#ifdef _DEBUG
    if(!g_verifyMode)
#endif
    {
      setFileTime(basics.filename, &(basics.created), 
                &(basics.accessed), &(basics.modified));

      setFileAttributes(basics.filename, basics.flags);
    }
  }

cleanup:
  if(record)
    ntfsx_record_free(record);

  ntfsx_cluster_release(&cluster);

  if(attribdata)
    ntfsx_attribute_free(attribdata);

  if(datarun)
    ntfsx_datarun_free(datarun);

  if(attrenum)
    ntfsx_attrib_enum_free(attrenum);

  if(ofile != -1)
    close(ofile);
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

  /* Read the MFT record */
	if(!ntfsx_record_read(record, sector, pi->device))
		err(1, "couldn't read mft");

  header = ntfsx_record_header(record);
  ASSERT(header);

  if(!(header->flags & kNTFS_RecFlagUse))
    errx(2, "invalid mft. marked as not in use");

  /* Load the MFT data runs */

  if(!ntfsx_mftmap_load(map, record, pi->device))
    err(1, "error reading in mft");

  /* Try and get a file name out of the header */

  processRecordFileBasics(pi, record, &basics);

  if(fcscmp(basics.filename, kNTFS_MFTName))
    errx(2, "invalid mft. wrong record");

  fprintf(stderr, "[Processing MFT...]\n");

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

void scroungeUsingRaw(partitioninfo* pi, uint64 skip)
{
	byte *buffer;
	size_t length;
	byte *bufsec;
	fchar_t dir[MAX_PATH + 1];
	uint64 sec;
	drivelocks locks;
	int64 pos;
	uint64 locked;
	size_t sz;
	uint32 magic = kNTFS_RecMagic;

	fprintf(stderr, "[Scrounging raw records...]\n");

	/* Save current directory away */
	fc_getcwd(dir, MAX_PATH);

	/* Get the locks ready */
	memset(&locks, 0, sizeof(locks));
	pi->locks = &locks;

	/* The memory buffer */
	length = kSectorSize * 2048;
	buffer = malloc(length);
	if(!buffer)
		errx(1, "out of memory");

	/* Loop through sectors */
	sec = pi->first + skip;
	while(sec < pi->end)
	{
#ifdef _WIN32
		fprintf(stderr, "sector: %I64u\r", sec);
#else
		fprintf(stderr, "sector: %llu\r", (unsigned long long)sec);
#endif

		/* Skip any locked sectors, already read */
		locked = checkLocationLock(&locks, sec);
		if(locked > 0)
		{
			sec += locked;
			continue;
		}

		/* Read a buffer size at this point */
		pos = SECTOR_TO_BYTES(sec);
		if(lseek64(pi->device, pos, SEEK_SET) == -1)
			errx(1, "can't seek device to sector");

		sz = read(pi->device, buffer, length);
		if(sz == -1 || sz < kSectorSize)
		{
			warn("can't read drive sector");

			/* Try again and go much slower */
			if(length != kSectorSize)
				length = kSectorSize;

			/* Already going slow, skip sector */
			else
				++sec;

			continue;
		}

		/* Now go through the read data */
		for(bufsec = buffer; bufsec < buffer + sz;
			bufsec += kSectorSize, ++sec)
		{
			/* Check beginning of sector for the magic signature */
			if(!memcmp(&magic, bufsec, sizeof(magic)))
			{
				/* Process the record */
				processMFTRecord(pi, sec, 0);
			}

			/* Move to right output directory */
			fc_chdir(dir);
		}
	}

	pi->locks = NULL;
}
