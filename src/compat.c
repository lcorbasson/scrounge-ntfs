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

#include "compat.h"
#include "usuals.h"

/*
 * We need to get a better check for this one.
 * Basically have to make a variable __progname if none
 * exist
 */

#ifndef HAVE_UNISTD_H

char* __progname = 0;
char prognamebuf[256];

void fixprogname()
{
	if(__progname == 0)
	{
		const char* beg = strrchr(_pgmptr, '\\');
		const char* temp = strrchr(_pgmptr, '/');
		beg = (beg > temp) ? beg : temp;
		beg = (beg) ? beg + 1 : _pgmptr;

		temp = strrchr(_pgmptr, '.');
		temp = (temp > beg) ? temp : _pgmptr + strlen(_pgmptr);

		if((temp - beg) > 255)
			temp = beg + 255;

		strncpy(prognamebuf, beg, temp - beg);
		prognamebuf[temp - beg] = 0;
		__progname = prognamebuf;
	}
}
#endif



#ifndef HAVE_GETOPT

int		opterr = 1,             /* if error message should be printed */
        optind = 1,             /* index into parent argv vector */
        optopt,                 /* character checked for validity */
        optreset;               /* reset getopt */
char*	optarg;

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""

/*
 * getopt --
 *      Parse argc/argv argument vector.
 */
int getopt(int nargc, char* const* nargv, const char* ostr)
{
    static char* place = EMSG;              /* option letter processing */
    char *oli;                              /* option letter list index */

	fixprogname();

	if(optreset || !*place)					/* update scanning pointer */
	{
		optreset = 0;
		if(optind >= nargc || *(place = nargv[optind]) != '-')
		{
			place = EMSG;
            return (-1);
        }

		if (place[1] && *++place == '-')       /* found "--" */
		{
			++optind;
            place = EMSG;
            return (-1);
        }
	}											/* option letter okay? */

    if ((optopt = (int)*place++) == (int)':' ||
		!(oli = strchr(ostr, optopt)))
	{
		/*
         * if the user didn't specify '-' as an option,
		 * assume it means -1.
		 */
		if(optopt == (int)'-')
			return (-1);
		if(!*place)
			++optind;
		if(opterr && *ostr != ':' && optopt != BADCH)
			(void)fprintf(stderr, "%s: illegal option -- %c\n", __progname, optopt);
				return(BADCH);
	}
	if (*++oli != ':')                  /* don't need argument */
	{
		optarg = NULL;
		if(!*place)
			++optind;
	}
	else                                /* need an argument */
	{
		if (*place)                     /* no white space */
			optarg = place;
		else if (nargc <= ++optind)		/* no arg */
		{
			place = EMSG;
			if (*ostr == ':')
				return (BADARG);
			if(opterr)
				(void)fprintf(stderr, "%s: option requires an argument -- %c\n", __progname, optopt);
					return(BADCH);
		}
		else                            /* white space */
			optarg = nargv[optind];

		place = EMSG;
		++optind;
	}
	return (optopt);                        /* dump back option letter */
}
#endif


#ifndef HAVE_ERR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static FILE *err_file; /* file to use for error output */
static void (*err_exit)(int);

/*
 * This is declared to take a `void *' so that the caller is not required
 * to include <stdio.h> first.  However, it is really a `FILE *', and the
 * manual page documents it as such.
 */
void err_set_file(void *fp)
{
	if (fp)
		err_file = fp;
	else
		err_file = stderr;
}

void err_set_exit(void (*ef)(int))
{
	err_exit = ef;
}

void err(int eval, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verrc(eval, errno, fmt, ap);
	va_end(ap);
}

void verr(int eval, const char *fmt, va_list ap)
{
	verrc(eval, errno, fmt, ap);
}

void errc(int eval, int code, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verrc(eval, code, fmt, ap);
	va_end(ap);
}

void verrc(int eval, int code, const char *fmt, va_list ap)
{
	fixprogname();
	if (err_file == 0)
		err_set_file((FILE *)0);
	fprintf(err_file, "%s: ", __progname);
	if (fmt != NULL) {
		vfprintf(err_file, fmt, ap);
		fprintf(err_file, ": ");
	}
	fprintf(err_file, "%s\n", strerror(code));
	if (err_exit)
		err_exit(eval);
	exit(eval);
}

void errx(int eval, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verrx(eval, fmt, ap);
	va_end(ap);
}

void verrx(int eval, const char *fmt, va_list ap)
{
	fixprogname();
	if (err_file == 0)
		err_set_file((FILE *)0);
	fprintf(err_file, "%s: ", __progname);
	if (fmt != NULL)
		vfprintf(err_file, fmt, ap);
	fprintf(err_file, "\n");
	if (err_exit)
		err_exit(eval);
	exit(eval);
}

void warn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vwarnc(errno, fmt, ap);
	va_end(ap);
}

void vwarn(const char *fmt, va_list ap)
{
	vwarnc(errno, fmt, ap);
}

void warnc(int code, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vwarnc(code, fmt, ap);
	va_end(ap);
}

void vwarnc(int code, const char *fmt, va_list ap)
{
	fixprogname();
	if (err_file == 0)
		err_set_file((FILE *)0);
	fprintf(err_file, "%s: ", __progname);
	if (fmt != NULL)
	{
		vfprintf(err_file, fmt, ap);
		fprintf(err_file, ": ");
	}
	fprintf(err_file, "%s\n", strerror(code));
}

void warnx(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vwarnx(fmt, ap);
	va_end(ap);
}

void vwarnx(const char *fmt, va_list ap)
{
	fixprogname();
	if(err_file == 0)
		err_set_file((FILE*)0);
	fprintf(err_file, "%s: ", __progname);
	if(fmt != NULL)
		vfprintf(err_file, fmt, ap);
	fprintf(err_file, "\n");
}

#endif


#ifndef HAVE_REALLOCF

void* reallocf(void* ptr, size_t size)
{
	void* ret = realloc(ptr, size);

	if(!ret && size)
		free(ptr);

	return ret;
}

#endif

#ifndef HAVE_ITOW
wchar_t* itow(int val, wchar_t* out, int radix)
{
  int mod;
  wchar_t temp;
  wchar_t* end = out;
  wchar_t* beg = out;

  if(val != 0)
  {
    /* If negative and decimal*/
    if(radix == 10 && val < 0)
      *beg++ = L'-';

    /* Convert in reverse order */
    while(val != 0)
    {
      mod = val % radix;
      val = val / radix;

      *end++ = (mod < 10) ? L'0' + mod : L'a' + mod - 10;
    }

    *end-- = 0;

    /* Reverse output string */
    while(end > beg)
    {
      temp = *end;
      *end = *beg;
      *beg = temp;
      ++beg;
      --end;
    }
  }
  else
  {
    beg[0] = L'0';
    beg[1] = 0;
  }

  return out;
}
#endif

#ifndef HAVE_ITOA
wchar_t* itow(int val, wchar_t* out, int radix)
{
  int mod;
  wchar_t temp;
  wchar_t* end = out;
  wchar_t* beg = out;

  if(val != 0)
  {
    /* If negative and decimal*/
    if(radix == 10 && val < 0)
      *beg++ = L'-';

    /* Convert in reverse order */
    while(val != 0)
    {
      mod = val % radix;
      val = val / radix;

      *end++ = (mod < 10) ? L'0' + mod : L'a' + mod - 10;
    }

    *end-- = 0;

    /* Reverse output string */
    while(end > beg)
    {
      temp = *end;
      *end = *beg;
      *beg = temp;
      ++beg;
      --end;
    }
  }
  else
  {
    beg[0] = L'0';
    beg[1] = 0;
  }

  return out;
}
#endif