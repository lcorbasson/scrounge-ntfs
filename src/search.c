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

  /* go through all sectors until we find an MFT record */
  /* that isn't the MFT mirror */

}
