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

#include <sys/types.h>

#ifdef _WIN32
  #include "config.win32.h"
#else
  #include "config.h"
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef _WIN32
#include <malloc.h>
#endif

#include "debug.h"
#include "compat.h"

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

#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#ifdef _WIN32
  #define UL(x)   x
#else
  #define UL(x)   x#LL
#endif

#endif /* __USUALS_H__ */
