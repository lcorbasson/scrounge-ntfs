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

#ifndef __MEMREF_H__
#define __MEMREF_H__

#ifdef _DEBUG

void* _refalloc_dbg(size_t sz);
void* _refadd_dbg(void* buf);
void _refrelease_dbg(void* buf);

#define refalloc	_refalloc_dbg
#define refadd		_refadd_dbg
#define refrelease	_refrelease_dbg

#else

void* _refalloc(size_t sz);
void* _refadd(void* buf);
void _refrelease(void* buf);

#define refalloc	_refalloc
#define refadd		_refadd
#define refrelease	_refrelease

#endif

#endif /* __MEMREF_H__ */
