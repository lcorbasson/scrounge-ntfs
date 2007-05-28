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

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include "usuals.h"
#include "ntfs.h"

const char kDriveName[] = "\\\\.\\PhysicalDrive%d";

#define ntfs_makefiletime(i64, ft) \
  ((ft).dwLowDateTime = LOWDWORD(i64), (ft).dwHighDateTime = HIGHDWORD(i64))

void makeDriveName(char* driveName, int i)
{
  wsprintf(driveName, kDriveName, i);
}

void setFileTime(fchar_t* filename, uint64* created, 
                  uint64* accessed, uint64* modified)
{
  FILETIME ftcr;
  FILETIME ftac;
  FILETIME ftmd;
  HANDLE file;

  ntfs_makefiletime(*created, ftcr);
	ntfs_makefiletime(*accessed, ftac);
	ntfs_makefiletime(*modified, ftmd);

	/* Write to the File */
	file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
  if(file == INVALID_HANDLE_VALUE)
  {
    warnx("couldn't set file time: " FC_PRINTF, filename);
    return;
  }

  if(!SetFileTime(file, &ftcr, &ftac, &ftmd))
    warnx("couldn't set file time: " FC_PRINTF, filename);

  CloseHandle(file);
}

void setFileAttributes(fchar_t* filename, uint32 flags)
{
  DWORD attributes;

  /* Attributes */
	if(flags & kNTFS_FileReadOnly)
		attributes |= FILE_ATTRIBUTE_READONLY;
	if(flags & kNTFS_FileHidden)
		attributes |= FILE_ATTRIBUTE_HIDDEN;
	if(flags & kNTFS_FileArchive)
		attributes |= FILE_ATTRIBUTE_ARCHIVE;
  if(flags & kNTFS_FileSystem)
		attributes |= FILE_ATTRIBUTE_SYSTEM;

  if(!SetFileAttributesW(filename, attributes))
    warnx("couldn't set file attributes: " FC_PRINTF, filename);
}
