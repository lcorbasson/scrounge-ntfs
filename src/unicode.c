
#include <wchar.h>

#include <wchar.h>
#include "sablo.h"

/*
 * Transcode UCS2 to UTF8. 
 *
 * Since the nature of the transformation is that the 
 * resulting length is unpredictable, this function
 * allocates it's own memory.
 */
char* unicode_transcode16to8(const wchar_t* src, size_t len)
{
  char* ret = NULL;
  size_t alloc = 0;
  size_t pos = 0;
  const wchar_t* c;
  const wchar_t* e;

  /* Allocate 1.25 times the length initially */
  alloc = len + (len / 4) + 1;
  ret = (char*)malloc(alloc * sizeof(char));
  if(!ret) return NULL;

  c = src;
  e = c + len;

  for( ; c < e; c++)
  {
    /* Make sure we have enough memory */
    if(pos + 4 >= alloc)
    {
      alloc += (len / 2) + 1;
      if(!(ret = (char*)reallocf(ret, alloc * sizeof(char))))
        return NULL;
    }

    /* Encode as one character */
    if(*c <= 0x007F)
    {
      ret[pos++] = (char)*c;
    }

    /* Encode as two characters */
    else if(*c <= 0x07FF)
    {
      ret[pos++] = (char)(192 | (*c >> 6));
      ret[pos++] = (char)(128 | (*c & 63));
    }

    /* Encode as three characters */
    else 
    {
      ret[pos++] = (char)(224 | (*c >> 12));
      ret[pos++] = (char)(128 | ((*c >> 6) & 63));
      ret[pos++] = (char)(128 | (*c & 63));
    }
  }

  ret[pos] = NULL;
  return ret;
}

/*
 * Transcode UTF-8 to UCS2
 * 
 * Since a semi predictable length of the resulting data is 
 * known, the caller should allocate the memory for this conversion.
 */
wchar_t* unicode_transcode8to16(const char* src, const wchar_t* out, size_t len)
{
  /* Note: out should always be at least as long as src in chars */

  size_t pos = 0;
  const char* c;
  const char* e;

  c = src;
  e = c + len;

  for( ; c < e; c++)
  {
    /* We never have to reallocate here. We will always
       be using the same or less number of output characters 
       than input chars. That's just the nature of the encoding. */
    
    /* First 4 bits set */
    if((c + 3) < e && 
       (c[0] & 0xF8) == 0xF0 && 
       (c[1] & 0xC0) == 0x80 &&
       (c[2] & 0xC0) == 0x80 &&
       (c[3] & 0xC0) == 0x80)
    {
      out[pos++] = (wchar_t)(((wchar_t)c[0] & 7) << 18 |
                             ((wchar_t)c[1] & 63) << 12 |
                             ((wchar_t)c[2] & 63) << 6 |
                             ((wchar_t)c[3] & 63));
      c += 3;
    }

    /* First 3 bits set */
    else if((c + 2) < e && 
            (c[0] & 0xF0) == 0xE0 &&
            (c[1] & 0xC0) == 0x80 &&
            (c[2] & 0xC0) == 0x80)
    {
      out[pos++] = (wchar_t)(((wchar_t)c[0] & 15) << 12 |
                             ((wchar_t)c[1] & 63) << 6 |
                             ((wchar_t)c[2] & 63));
      c += 2;
    }

    /* First 2 bits set */
    else if((c + 1) < e && 
            (c[0] & 0xE0) == 0xC0 &&
            (c[1] & 0xC0) == 0x80)
    {
      out[pos++] = (wchar_t)(((wchar_t)c[0] & 31) << 6 |
                             ((wchar_t)c[1] & 63));
      c += 1;
    }

    /* First bit set */
    else if(!(c[0] & 0x80))  
    {
      out[pos++] = (wchar_t)c[0];
    }

    /* Invalid encoding */
    else
    {
      out[pos++] = L'?';
    }
  }

  out[pos] = NULL;
  return out;
}

