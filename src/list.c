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
#include "drive.h"
#include "ntfs.h"

const char kPrintData[]		= "\
    Start Sector    End Sector      Cluster Size    MFT Offset    \n\
==================================================================\n\
";

const char kPrintDrive[]		= "\nDrive: %u\n";
const char kPrintDrivePath[]		= "\nDrive: %s\n";
const char kPrintDriveInfo[]	= "    %-15u %-15u ";
const char kPrintNTFSInfo[]		= "%-15u %-15u";

int printNTFSInfo(int dd, uint64 tblSector)
{
	byte sector[kSectorSize];
  int64 pos;
  size_t sz;
  ntfs_bootsector* boot;

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
		printf(kPrintNTFSInfo, boot->secPerClus, boot->offMFT * boot->secPerClus);

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
			else if(!mbr.partitions[i].system == kPartition_Invalid)
			{
				printf(kPrintDriveInfo, (uint32)tblSector + mbr.partitions[i].startsec, (uint32)tblSector + mbr.partitions[i].endsec);
				printNTFSInfo(dd, tblSector + (uint64)mbr.partitions[i].startsec);
			}
		}
	}

	return 0;
}


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

    dd = open(driveName, _O_BINARY | _O_RDONLY | OPEN_LARGE_OPTS);
    if(dd != -1)
    {
			printf(kPrintDrive, i);
			printPartitionInfo(dd, 0);
			close(dd);
		}
	}
}

void scroungeListDrive(char* drive)
{
  int dd = open(driveName, _O_BINARY | _O_RDONLY | OPEN_LARGE_OPTS;
  if(dd == -1)
    err(1, "couldn't open drive: %s", driveName);

  printf(kPrintDrivePath, driveName);
  printPartitionInfo(dd, 0);
  close(dd);
}
