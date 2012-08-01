/*
 * x_string.c
 *
 *  Created on: Aug 7, 2011
 *      Author: artemka
 */

#include <sys/types.h>
#include <stdlib.h>

#undef DEBUG_PRFX
//#define DEBUG_PRFX "[x_string] "
#include "x_types.h"
#include "x_string.h"
#include "x_lib.h"

static __inline__ void
__x_string_check_capacity(x_string *xstr, unsigned int extracapacity)
{
  if (xstr->capacity <= (xstr->pos + extracapacity + 1))
    {
      xstr->capacity += X_STR_BUFSIZ + extracapacity + 1;
      xstr->cbuf = x_realloc(xstr->cbuf, xstr->capacity);
    }
  return;
}

/**
 * constructor
 */
x_string *
x_string_new(void)
{
  x_string *xstr;
  ENTER;
  xstr = x_malloc(sizeof(x_string));
  if (xstr)
    {
      xstr->pos = 0;
      xstr->capacity = 0;
    }
  EXIT;
  return xstr;
}

/**
 *
 */
EXPORT_SYMBOL void
x_string_clear(x_string *xstr)
{
  ENTER;
  if ((xstr->capacity ^ X_STR_BUFSIZ))
    xstr->cbuf = x_realloc(xstr->cbuf, X_STR_BUFSIZ);
  xstr->capacity = X_STR_BUFSIZ;
  xstr->pos = 0;
  EXIT;
  return;
}

/**
 *
 */
EXPORT_SYMBOL void
x_string_trunc(x_string *xstr)
{
  ENTER;
  if (xstr->cbuf)
    x_free(xstr->cbuf);
  xstr->cbuf = NULL;
  xstr->capacity = 0;
  xstr->pos = 0;
  EXIT;
  return;
}

/**
 * destructor
 */
void
x_string_delete(x_string *xstr)
{
  if (xstr)
    {
      if (xstr->cbuf)
        x_free(xstr->cbuf);
      x_free(xstr);
    }
  return;
}

/**
 * Adds character at end of string
 * @todo x_string reallocation mechanism should be changed
 */
int32_t
x_string_putc(x_string *xstr, char c)
{
  __x_string_check_capacity(xstr, 1);
  /* append new character */
  xstr->cbuf[xstr->pos++] = c;
  /* set endline character */
  xstr->cbuf[xstr->pos] = '\0';
  return 0;
}

/**
 * Adds character at end of string
 */
int32_t
x_string_write(x_string *xstr, const char *s, size_t siz)
{
  size_t i = 0;

  __x_string_check_capacity(xstr, (unsigned int) siz);

  /* copying */
#if 0
  while (i < siz)
    xstr->cbuf[xstr->pos++] = s[i++];
#else
  x_memcpy(&xstr->cbuf[xstr->pos],s,siz);
  xstr->pos += siz;
#endif

  /* set endline character */
  xstr->cbuf[xstr->pos] = '\0';

  return (int32_t) i;
}
