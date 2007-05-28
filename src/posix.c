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

#include <sys/stat.h>
#include <unistd.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/* NOTE: This file assumes that FC_WIDE is off */

/* The NTFS file time is a a 64-bit value representing the 
   number of 100-nanosecond intervals since January 1, 1601 */

/* The unix epoch in NTFS ft */
#define UNIX_EPOCH    116444736000000000LL

void ntfs_maketvs(uint64* ft, struct timeval* tv)
{
  /* Anything before the unix epoch is stupid */
  if(*ft < UNIX_EPOCH)
  {
    tv->tv_sec = 0;
    tv->tv_usec = 0;
  }

  /* Anything later than we can represent is a bummer */
  else if(sizeof(tv->tv_sec) == 32)
  {
    tv->tv_sec = 0x7FFFFFFF;
    tv->tv_usec = 0x7FFFFFFF;
  }

  /* Now convert the valid range of dates */
  else
  {
    uint mod = (*ft % 1000000000);
    tv->tv_sec = ((*ft - mod) / 1000000000);
    tv->tv_usec = mod / 1000;
  }
};

void setFileTime(fchar_t* filename, uint64* created, 
                  uint64* accessed, uint64* modified)
{
  int r;
  struct timeval tvs[2];
  char* encoded;

  ntfs_maketvs(accessed, tvs);
  ntfs_maketvs(modified, tvs + 1);
  if(utimes(filename, tvs) == -1)
    warn("couldn't set file times on: %s", encoded);
}

void setFileAttributes(fchar_t* filename, uint32 flags)
{
  char* encoded;
  struct stat st;

  if(flags & kNTFS_FileReadOnly)
  {
    if(stat(filename, &st) == -1)
    {
      warn("couldn't read file status for: " FC_PRINTF, encoded);
    }
    else
    {
      st.st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	fprintf(stderr, "mode: %x", st.st_mode);

      if(chmod(filename, st.st_mode) == -1)
        warn("couldn't set file attributes for: " FC_PRINTF, encoded);
    }
  }
}


