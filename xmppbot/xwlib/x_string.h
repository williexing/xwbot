/*
 * x_string.h
 *
 *  Created on: Aug 8, 2011
 *      Author: artemka
 */

#ifndef X_STRING_H_
#define X_STRING_H_

#include <sys/types.h>
#include "x_lib.h"

/**
 * Self growing strings
 */
typedef struct x_string__
{
  u_int32_t capacity;   /* current buffer capacity */
  u_int32_t pos;        /* current position (last char) */
  char *cbuf;           /* allocated character buffer */
} x_string;

typedef char* x_string_t;

#define X_STRING_STATIC_INIT {0,0,NULL};

EXPORT_SYMBOL int32_t x_string_putc(x_string *xstr, char c);
EXPORT_SYMBOL int32_t x_string_write(x_string *xstr, const char *s, size_t siz);
EXPORT_SYMBOL x_string *x_string_new(void);
EXPORT_SYMBOL void x_string_clear(x_string *xstr);
EXPORT_SYMBOL void x_string_trunc(x_string *xstr);
EXPORT_SYMBOL void x_string_delete(x_string *xstr);
EXPORT_SYMBOL void x_string_rew(x_string *xstr);

/*---------------------------------------------
 * New version of string API
 *---------------------------------------------*/
#ifdef USE_NEW_STRING_API
#define X_STRING(x) {sizeof(x),x}
#define _XS(x) {sizeof(x),x}
#else
#define X_STRING(x) ((x_string_t)x)
#define _XS(x) ((x_string_t)x)
#endif

#endif /* X_STRING_H_ */
