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

void scroungeSearch(partitioninfo* pi)
{
  fprintf(stderr, "[Performing NTFS partition search...]");
  errx(1, "search functionality not implemented yet.");

  /* 
   * go through and look for an $MFTMirr file record. First of all
   * these should be exactly 4096 bytes long. This allows us 
   * to discover the length of a cluster, using both the data
   * runs in the $MFTMirr and the cbAllocated.
   *
   * Determine if it's the $MFTMirr or $MFT we're looking at
   * 
   * Next using this cluster information we can discover the
   * beginning of the partition, 
   */


}
