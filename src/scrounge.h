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

#ifndef __SCROUNGE_H__
#define __SCROUNGE_H__

#include "drive.h"

void scroungeSearch(partitioninfo* pi);
void scroungeList();
void scroungeListDrive(char* drive);
void scroungeUsingMFT(partitioninfo* pi);
void scroungeUsingRaw(partitioninfo* pi);

/* For compatibility */
void setFileAttributes(fchar_t* filename, uint32 flags);
void setFileTime(fchar_t* filename, uint64* created, uint64* accessed, uint64* modified);

#endif /* __SCROUNGE_H__ */
