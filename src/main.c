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

#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scrounge.h"
#include "compat.h"

const char kPrintHelp[]       = "\
usage: scrounge -l                                                   \n\
  List all drive partition information.                              \n\
                                                                     \n\
usage: scrounge [-d drive] -s                                        \n\
  Search drive for NTFS partitions.                                  \n\
                                                                     \n\
usage: scrounge [-d drive] [-m mftoffset] [-c clustersize] [-o outdir] start end  \n\
  Scrounge data from a partition                                     \n\
  -d         Drive number                                            \n\
  -m         Offset to mft (in sectors)                              \n\
  -c         Cluster size (in sectors, default of 8)                 \n\
  -o         Directory to put scrounged files in                     \n\
  start      First sector of partition                               \n\
  end        Last sector of partition                                \n\
                                                                     \n\
";

#define MODE_SCROUNGE 1
#define MODE_LIST     2
#define MODE_SEARCH   3 

/* Forward decls */
int usage();

int main(int argc, char* argv[])
{
  int ch = 0;
  int temp = 0;
  int mode = 0;
  int raw = 0;
  int drive = 0;
  partitioninfo pi;
  char driveName[MAX_PATH];

  memset(&pi, 0, sizeof(pi));

  /* TODO: We need to be able to autodetect the cluster size */
  pi.cluster = 8;

  while((ch = getopt(argc, argv, "c:d:hlm:o:s")) != -1)
  {
    switch(ch)
    {

    /* cluster size */
    case 'c':
      {
        temp = atoi(optarg);
        
        /* TODO: Check this range */
        if(temp <= 0 || temp > 128)
          errx(2, "invalid cluster size (must be between 1 and 128)");

        pi.cluster = temp;
        mode = MODE_SCROUNGE;
      }
      break;

    /* drive number */
    case 'd':
      {
        temp = atoi(optarg);

        /* TODO: Check this range */
        if(temp < 0 || temp > 128)
          errx(2, "invalid drive number (must be between 0 and 128)");

        drive = temp;
      }
      break;

    /* help mode */
    case 'h':
      usage();
      break;

    /* list mode */
    case 'l':
      {
        if(mode == MODE_SCROUNGE)
          errx(2, "invalid -l argument in scrounge mode");

        mode = MODE_LIST;
      }
      break;

    /* mft offset */
    case 'm':
      {
        temp = atoi(optarg);

        /* TODO: Check this range */
        if(temp < 0)
          errx(2, "invalid mft offset (must be positive)");

        pi.mft = temp;
        mode = MODE_SCROUNGE;
      }
      break;

    /* output directory */
    case 'o':
      if(chdir(optarg) == -1)
        err(2, "couldn't change to output directory");
      break;

    /* search mode */
    case 's':
      {
        if(mode == MODE_SCROUNGE)
          errx(2, "invalid -s argument in scrounge mode");

        mode = MODE_SEARCH;
      }
      break;

    default:
      ASSERT(false);
      break;
    }

    if(mode != MODE_SCROUNGE && mode != 0)
      break;
  }

  argc -= optind;
  argv += optind;

  if(mode == MODE_SCROUNGE || mode == 0)
  {
    /* Get the sectors */

    if(argc < 2)
      errx(2, "must specify start and end sector of partition");

    if(argc > 2)
      warnx("ignoring extra arguments");

    temp = atoi(argv[0]);
    if(temp < 0)
      errx(2, "invalid start sector (must be positive)");

    pi.first = temp;

    temp = atoi(argv[1]);
    if(temp < 0 || ((unsigned int)temp) <= pi.first)
      errx(2, "invalid end sector (must be positive and greater than first)");

    pi.end = temp;

    makeDriveName(driveName, drive);

    pi.device = open(driveName, _O_BINARY | _O_RDONLY);
    if(pi.device == -1)
      err(1, "couldn't open drive");

    /* Use mft type search */
    if(pi.mft != 0)
    {
      scroungeUsingMFT(&pi);
    }

    /* Otherwise it's a raw search */
    else
    {
      warn("Scrounging via raw search. Directory info will be discarded.");
      scroungeUsingRaw(&pi);
    }
  }

  else
  {
    if(argc > 0)
      warnx("ignoring extra arguments");

    /* List partition and drive info */
    if(mode == MODE_LIST)
      scroungeList();

    /* Search for NTFS partitions */
    if(mode == MODE_SEARCH)
      scroungeSearch(&pi);
  }

  return 0;
}

int usage()
{
  fprintf(stderr, "%s", kPrintHelp);
  exit(2);
}
