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

#ifndef __USUALS_H__
#define __USUALS_H__

#include "debug.h"
#include "compat.h"
#include <errno.h>

typedef unsigned __int64 uint64;
typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char byte;

typedef signed __int64 int64;
typedef signed long int32;
typedef signed short int16;

#ifndef NULL
#define NULL 0
#endif

#define RETWARNBX(s) { ret = false; warnx(s); goto cleanup; }
#define RETWARNB(s) { ret = false; warn(s); goto cleanup; }
#define RETWARNX(s) { warnx(s); goto cleanup; }
#define RETWARN(s) { warn(s); goto cleanup; }
#define RETURN goto cleanup

#define HIGHDWORD(i64)	(DWORD)((i64) >> 32)
#define LOWDWORD(i64) (DWORD)((i64) & 0xFFFFFFFF)


#define INTERSECTS(b1, e1, b2, e2) \
	((b1) < (e2) && (e1) > (b2))

#endif /* __USUALS_H__ */
