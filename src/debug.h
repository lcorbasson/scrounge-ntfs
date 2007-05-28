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

#ifdef _DEBUG

#include <stdarg.h>

#ifndef ASSERT
	#ifdef ATLASSERT
		#define ASSERT ATLASSERT
	#else
		#include <assert.h>
		#define ASSERT assert
	#endif
#endif

#ifndef VERIFY
#define VERIFY(f) ASSERT(f)
#endif

#ifndef DEBUG_ONLY
#define DEBUG_ONLY(f)      (f)
#endif


#else /* !DEBUG */

#ifndef ASSERT
#define ASSERT(f)          ((void)0)
#endif

#ifndef VERIFY
#define VERIFY(f)          ((void)(f))
#endif

#ifndef DEBUG_ONLY
#define DEBUG_ONLY(f)      ((void)0)
#endif

#endif /* _DEBUG */
