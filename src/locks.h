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

#ifndef __LOCKS_H__
#define __LOCKS_H__

#include "usuals.h"

struct drivelock;
typedef struct _drivelocks
{
  struct drivelock* _locks;
  uint32 _count;
 	uint32 _current;
}
drivelocks;

void addLocationLock(drivelocks* locks, uint64 beg, uint64 end);
bool checkLocationLock(drivelocks* locks, uint64 sec);

#ifdef _DEBUG
void dumpLocationLocks(drivelocks* locks);
#endif

#endif /* __LOCKS_H__ */
