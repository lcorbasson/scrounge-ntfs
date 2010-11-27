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
#include "drive.h"
#include "ntfs.h"

const char kPrintData[]		= "\
    Start Sector    End Sector      Cluster Size    MFT Offset    \n\
==================================================================\n\
";

const char kPrintDrive[]		= "\nDrive: %u\n";
const char kPrintDrivePath[]		= "\nDrive: %s\n";

int printNTFSInfo(int dd, uint64 tblSector)
{
	byte sector[kSectorSize];
  int64 pos;
  size_t sz;
  ntfs_bootsector* boot;

  /* TODO: Check the range and don't print if out of range */
	pos = SECTOR_TO_BYTES(tblSector);

  if(lseek64(dd, pos, SEEK_SET) == -1)
    err(1, "couldn't seek drive");

  sz = read(dd, &sector, kSectorSize);
  if(sz == -1)
    err(1, "couldn't read drive");

  if(sz != kSectorSize)
    errx(1, "unexpected end of drive");

  boot = (ntfs_bootsector*)sector;
	if(!memcmp(boot->sysId, kNTFS_SysId, sizeof(boot->sysId)))
	{
#ifdef _WIN32
		printf("%-15u %-15I64u", (unsigned int)boot->secPerClus,
		       boot->offMFT * (uint64)boot->secPerClus);
#else
		printf("%-15u %-15llu", (unsigned int)boot->secPerClus,
		       (unsigned long long)(boot->offMFT * (uint64)boot->secPerClus));
#endif
	}

	printf("\n");
	return 0;
}

int printPartitionInfo(int dd, uint64 tblSector)
{
	drive_mbr mbr;
  int64 pos;
  size_t sz;
  int i;

	ASSERT(sizeof(drive_mbr) == kSectorSize);

	pos = SECTOR_TO_BYTES(tblSector);

  if(lseek64(dd, pos, SEEK_SET) == -1)
    err(1, "couldn't seek drive");

  sz = read(dd, &mbr, sizeof(drive_mbr));
  if(sz == -1)
    err(1, "couldn't read drive");

  if(sz != sizeof(drive_mbr))
    errx(1, "unexpected end of drive");

	if(mbr.sig == kMBR_Sig)
	{
		for(i = 0; i < 4; i++)
		{
			if(mbr.partitions[i].system == kPartition_Extended ||
			   mbr.partitions[i].system == kPartition_ExtendedLBA)
			{
				printPartitionInfo(dd, tblSector + mbr.partitions[i].startsec);
			}
			else if(tblSector + mbr.partitions[i].startsec == 1UL &&
			        tblSector + mbr.partitions[i].endsec == 0xFFFFFFFFUL)
			{
				printf("    large partition, perhaps using gpt\n");
			}
			else if(!mbr.partitions[i].system == kPartition_Invalid)
			{
#ifdef _WIN32
				printf("    %-15I64u %-15I64u ",
				       tblSector + mbr.partitions[i].startsec,
				       tblSector + mbr.partitions[i].endsec);
#else
				printf("    %-15llu %-15llu ",
				       (unsigned long long)(tblSector + mbr.partitions[i].startsec),
				       (unsigned long long)tblSector + mbr.partitions[i].endsec));
#endif

				printNTFSInfo(dd, tblSector + (uint64)mbr.partitions[i].startsec);
			}
		}
	}

	return 0;
}

#ifdef _WIN32
void scroungeList()
{
	char driveName[MAX_PATH];
  int dd = -1;
  int i;

  printf(kPrintData);

	/* LIMIT: 256 Drives */
	for(i = 0; i < 0x100; i++)
	{
    makeDriveName(driveName, i);

    dd = open(driveName, O_BINARY | O_RDONLY | OPEN_LARGE_OPTS);
    if(dd != -1)
    {
			printf(kPrintDrive, i);
			printPartitionInfo(dd, 0);
			close(dd);
		}
	}
}
#endif

void scroungeListDrive(char* drive)
{
  int dd = open(drive, O_BINARY | O_RDONLY | OPEN_LARGE_OPTS);
  if(dd == -1)
    err(1, "couldn't open drive: %s", drive);

  printf(kPrintData);
  printf(kPrintDrivePath, drive);
  printPartitionInfo(dd, 0);
  close(dd);
}
