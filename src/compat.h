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

#ifndef _COMPAT_H_
#define _COMPAT_H_

/* Force use of win32 configuration if compiling there */
#ifdef _WIN32
#include "config.win32.h"
#else
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#endif

#include <sys/types.h>

#ifndef MAX_PATH
#ifdef _MAX_PATH
#define MAX_PATH _MAX_PATH
#else
#define MAX_PATH 256
#endif
#endif

#ifndef HAVE_STDARG_H
#error ERROR: Must have a working stdarg.h header
#else
#include <stdarg.h>
#endif

#ifndef HAVE_GETCWD
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#define HAVE_GETCWD
#else
#error ERROR: Must have a working 'getcwd' function
#endif
#endif

#ifndef HAVE_CHDIR
#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#define HAVE_CHDIR
#else
#error ERROR: Must have a working 'chdir' function
#endif
#endif

#ifndef NULL
#define NULL	(void*)0
#endif

#ifndef HAVE_BOOL
typedef unsigned char bool;
#define false	0x00
#define true	0x01
#endif

#ifndef HAVE_BYTE
typedef unsigned char byte;
#endif

#ifndef HAVE_UINT
typedef unsigned int uint;
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifndef HAVE_UINT64
  #ifdef HAVE_UINT64_T
    typedef uint64_t uint64;
  #else
    #ifdef _WIN32
      typedef unsigned __int64 uint64;
    #else
      #error ERROR: Must have a compiler that can handle 64 bit integers
    #endif
  #endif
#endif

#ifndef HAVE_INT64
  #ifdef HAVE_INT64_T
    typedef int64_t int64;
  #else
    #ifdef _WIN32
      typedef signed __int64 int64;
    #else
      #error ERROR: Must have a compiler that can handle 64 bit integers
    #endif
  #endif
#endif

#ifndef HAVE_UINT32
  #ifdef HAVE_UINT32_T
    typedef uint32_t uint32;
  #else
    #ifdef _WIN32
      typedef unsigned __int32 uint32;
    #else
      #error ERROR: Couldnt determine a 32 bit integer
    #endif
  #endif
#endif

#ifndef HAVE_INT32
  #ifdef HAVE_INT32_T
    typedef int32_t int32;
  #else
    #ifdef _WIN32
      typedef signed __int32 int32;
    #else
      #error ERROR: Couldnt determine a 32 bit integer
    #endif
  #endif
#endif

#ifndef HAVE_UINT16
  #ifdef HAVE_UINT16_T
    typedef uint16_t uint16;
  #else
    #ifdef _WIN32
      typedef unsigned __int16 uint16;
    #else
      #error ERROR: Couldnt determine a 16 bit integer
    #endif
  #endif
#endif

#ifndef HAVE_INT16
  #ifdef HAVE_INT16_T
    typedef int16_t int16;
  #else
    #ifdef _WIN32
      typedef signed __int16 int16;
    #else
      #error ERROR: Couldnt determine a 16 bit integer
    #endif
  #endif
#endif


#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef HAVE_GETOPT
extern char* optarg;
extern int optind, opterr, optopt;
int getopt(int nargc, char* const* nargv, const char* ostr);
#endif

#ifndef HAVE_ERR_H
#include <stdarg.h>
void err_set_file(void *fp);
void err_set_exit(void (*ef)(int));
void err(int eval, const char *fmt, ...);
void verr(int eval, const char *fmt, va_list ap);
void errc(int eval, int code, const char *fmt, ...);
void verrc(int eval, int code, const char *fmt, va_list ap);
void errx(int eval, const char *fmt, ...);
void verrx(int eval, const char *fmt, va_list ap);
void warn(const char *fmt, ...);
void vwarn(const char *fmt, va_list ap);
void warnc(int code, const char *fmt, ...);
void vwarnc(int code, const char *fmt, va_list ap);
void warnx(const char *fmt, ...);
void vwarnx(const char *fmt, va_list ap);
#endif

#ifndef HAVE_REALLOCF
void* reallocf(void* p, size_t sz);
#endif

/* Some number conversion stuff */
#include <wchar.h>

#ifndef HAVE_ITOW
  #ifdef _WIN32
    #define itow _itow
    #define HAVE_ITOW 1
  #else
    wchar_t* itow(int v, wchar_t* s, int r);
  #endif
#endif

#ifndef HAVE_ITOA
  #ifdef _WIN32
    #define itoa _itoa
    #define HAVE_ITOA 1
  #else
    char* itoa(int v, char* s, int r);
  #endif
#endif


/*
 * Depending on the OS we use different width characters
 * for file names and file access. Before enabling wide
 * file access for an OS the printf needs to be able to
 * handle wide chars (ie: %S) and there should be wide
 * char file access functions
 */

#ifdef _WIN32
  /* On windows we use UCS2 */
  typedef wchar_t fchar_t;
  #define FC_WIDE 1
  #define FC_PRINTF "%S"
#else
  /* Everywhere else we use UTF-8 */
  typedef char fchar_t;
  #undef FC_WIDE
  #define FC_PRINTF "%s"
#endif

#ifdef FC_WIDE

  /* An OS that handles wide char file access */

  #ifdef HAVE_WOPEN
    #define fc_open wopen
  #else
    #ifdef _WIN32
      #define fc_open _wopen
    #else
      #error Set for wide file access, but no wide open
    #endif
  #endif

  #ifdef HAVE_WCHDIR
    #define fc_chdir wchdir
  #else
    #ifdef _WIN32
      #define fc_chdir _wchdir
    #else
      #error Set for wide file access but no wide chdir
    #endif
  #endif

  #ifdef HAVE_WMKDIR
    #define fc_mkdir wmkdir
  #else
    #ifdef _WIN32
      #define fc_mkdir _wmkdir
    #else
      #error Set for wide file access but no wide mkdir
    #endif
  #endif

  #ifdef HAVE_WGETCWD
    #define fc_getcwd wgetcwd
  #else
    #ifdef _WIN32
      #define fc_getcwd _wgetcwd
    #else
      #error Set for wide file access but no wide getcwd
    #endif
  #endif

  #define fcscpy wcscpy
  #define fcscat wcscat
  #define fcsncpy wcsncpy
  #define fcslen wcslen
  #define fcscmp wcscmp
  #define itofc itow

  #define FC_DOT L"."

#else

  /* OSs without wide char file access */

  #define fc_open open
  #define fc_chdir chdir
  #define fc_mkdir mkdir
  #define fc_getcwd getcwd

  #define fcscpy strcpy
  #define fcsncpy strncpy
  #define fcscat strcat
  #define fcslen strlen
  #define fcscmp strcmp
  #define itofc itoa

  #define FC_DOT "."

#endif



/* 64 bit file handling stuff */

#ifndef HAVE_LSEEK64
  #ifdef _WIN32
    #define lseek64 _lseeki64
  #else
    #if SIZEOF_OFF_T == 8
      #define lseek64 lseek
    #else
      #error ERROR: Must have a working 64 bit seek function
    #endif
  #endif
#endif

#include <fcntl.h>
#ifdef O_LARGEFILE
  #define OPEN_LARGE_OPTS O_LARGEFILE
#else
  #define OPEN_LARGE_OPTS 0
#endif

#ifndef O_BINARY
  #ifdef _O_BINARY
    #define O_BINARY _O_BINARY
  #else
    #define O_BINARY 0
  #endif
#endif

#ifndef O_RDONLY
  #ifdef _O_RDONLY
    #define O_RDONLY _O_RDONLY
  #else
    #error ERROR: No O_RDONLY flag found
  #endif
#endif


#endif /* _COMPAT_H_ */
