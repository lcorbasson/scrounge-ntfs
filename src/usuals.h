// 
// AUTHOR
// N. Nielsen
//
// VERSION
// 0.7
// 
// LICENSE
// This software is in the public domain.
//
// The software is provided "as is", without warranty of any kind,
// express or implied, including but not limited to the warranties
// of merchantability, fitness for a particular purpose, and
// noninfringement. In no event shall the author(s) be liable for any
// claim, damages, or other liability, whether in an action of
// contract, tort, or otherwise, arising from, out of, or in connection
// with the software or the use or other dealings in the software.
// 
// SUPPORT
// Send bug reports to: <nielsen@memberwebs.com>
//

#ifndef __USUALS_H__20010822
#define __USUALS_H__20010822

#include <win32/debug.h>

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


#define HIGHDWORD(i64)	(DWORD)((i64) >> 32)
#define LOWDWORD(i64) (DWORD)((i64) & 0xFFFFFFFF)

#endif //__USUALS_H__20010822
