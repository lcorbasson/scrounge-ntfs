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
#include "../config.win32.h"
#else
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#endif

#ifndef MAX_PATH
  #ifdef _MAX_PATH
    #define MAX_PATH _MAX_PATH
  #else
    #define MAX_PATH 256
  #endif
#endif

#ifndef HAVE_STDARG_H
#error ERROR: Must have a working <stdarg.h> header
#else
#include <stdarg.h>
#endif


#ifndef HAVE_STAT
#ifdef _WIN32
#define S_IFDIR _S_IFDIR
#define HAVE_STAT
#else
#error ERROR: Must have a working 'stat' function
#endif
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

#ifndef HAVE_TOLOWER
int tolower(int c);
#endif

#ifndef HAVE_STRDUP
char* strdup(const char* str);
#endif

#ifndef HAVE_STRNDUP
char* strndup(const char* str, size_t cnt);
#endif

#ifndef HAVE_STRCASESTR
char* strcasestr(const char* big, const char* little);
#endif

#ifndef HAVE_STRCASECMP
#ifdef HAVE_STRICMP
#define strcasecmp stricmp
#else
#error ERROR: Must have either 'strcasecmp' or 'stricmp'
#endif
#endif

#ifndef HAVE_STRCASECMP
#ifdef HAVE_STRICMP
#define strncasecmp strnicmp
#else
#error ERROR: Must have either 'strncasecmp' or 'strnicmp'
#endif
#endif


#ifndef NULL
#define NULL	(void*)0
#endif

#ifndef __cplusplus
typedef unsigned char bool;
#define false	0x00
#define true	0x01
#endif

#ifndef HAVE_BYTE
typedef unsigned char byte;
#define HAVE_BYTE
#endif

#ifndef HAVE_UINT
typedef unsigned int uint;
#define HAVE_UINT
#endif

#ifndef HAVE_STRLCPY
void strlcpy(char* dest, const char* src, size_t count);
#endif

#ifndef HAVE_STRLCAT
void strlcat(char* dest, const char* src, size_t count);
#endif

#ifndef HAVE_VSNPRINTF

#ifdef _WIN32
#define vsnprintf _vsnprintf
#define HAVE_VSNPRINTF
#else
#ifndef HAVE_VASPRINTF
#error ERROR: Must have a working 'vsnprintf' or 'vasprintf' function
#endif
#endif
#endif

#ifndef HAVE_VASPRINTF
int vasprintf(char** ret, const char* format, va_list vl);
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
void* reallocf(void* ptr, size_t size);
#endif

#endif
