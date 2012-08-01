/*
 * Copyright (c) 2011, artemka
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * xwbot
 * Created on: Jun 7, 2011
 *     Author: artemka
 *
 */

#ifndef X_PARSER_H_
#define X_PARSER_H_

#include <sys/types.h>
#include <xwlib/x_string.h>
#include <xwlib/x_obj.h>

/*
 * PUSH PARSER DEFINITIONS
 */
struct x_push_parser;
typedef int32_t
(*x_putc_ft)(struct x_push_parser *p, const char c);

/** initial stack size */
#define X_INI_STACK_SIZE 64
#define X_INI_STACK_TYPE void *

#define PP_ATTR_F (1<<0)

/**
 * Xml push parser
 */
struct x_push_parser
{
  /** superclass for x_push_parser */
  x_object obj;

  /** pattern object register */
  struct ht_cell attrs;

  /* tag register 0, usually used for tagname */
  x_string r0;

  /* tag register 1 */
  x_string r1;

  /* tag register 2 */
  x_string r2;

  /* general purpose registers */
  void *g1;
  void *g2;
  void *g3;
  void *g4;

  /* control registers */
  u_int32_t cr0;
  u_int32_t cr1;

  /** state pointer */
  x_putc_ft r_putc;

  /** state pointer */
  void (*r_reset)(struct x_push_parser *);

  /** current xobject */
  x_object *r_obj;

  /** stack */
  void **sp;

  /** stack base */
  void **bp;

  /* private data */
  void *parserpriv;

  /* allocates object in object tree by given pattern */
  int (*xobjopen)(const char *,struct ht_cell *, void *);

  /* allocates object in object tree by given pattern */
  int (*xputchar)(char c, void *);

  /* called when object parsing finished */
  int (*xobjclose)(const char *, void *);

  /* called when new xml document found */
  int (*xobjreset)(void *);

  /* callback data */
  void *cbdata;
};

enum
{
  X_PP_INI = 0,
  /** cr1 FLAG indicating that parser in a 'value' phase of
   * key/value parsing  */
  X_PP_KV_VAL = 1,
  X_PP_KV_LAV = 2,
  /** cr1 FLAG indicating unstable phase of k/v parsing  */
  X_PP_KV_WANT_VAL,
  X_PP_KV_KEY,
  /** cr1 FLAG indicating object opening state */
  X_PP_OBJ_OPENING,
  /** cr1 FLAG indicating object closing state */
  X_PP_OBJ_CLOSING,
};

/**
 * Initializes xml push parser
 */
EXPORT_SYMBOL void x_parser_init(struct x_push_parser *pp);
int x_p_putc(struct x_push_parser *p, const char c);
EXPORT_SYMBOL int x_parser_object_to_string(x_object *xobj, x_string *xstr);
EXPORT_SYMBOL int x_parser_object_to_fd(x_object *xobj, int fd);

#endif /* X_PARSER_H_ */
