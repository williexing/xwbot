/*
 * Copyright (c) 2010, artemka
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
 * CXmppAgent
 * Created on: Sep 22, 2010
 *     Author: artemka
 *
 */

#include <string.h>
#include <ezxml.h>
#include <xmppagent.h>
#include <xforms.h>

char *
xforms_get_type(ezxml_t _form)
{
  return (char *) ezxml_attr(_form, "type");
}

/**
 * Returns variable value in given xform
 */
char *
xforms_get_value_of(ezxml_t _form, const char *var)
{
  ezxml_t stnz1, stnz2;
  char *val = NULL;

  if (!var || !_form || strcasecmp(_form->name, "x"))
    return NULL;

  for (stnz1 = ezxml_get(_form, "field", -1); stnz1; stnz1 = stnz1->next)
    {
      if (!strcmp(var, ezxml_attr(stnz1, "var")))
        {
          stnz2 = ezxml_get(stnz1, "value", -1);
          if (stnz2)
            val = stnz2->txt;
          break;
        }
    }

  return val;
}

/**
 *
 */
char *
xforms_set_val(ezxml_t _form, const char *var, char *value)
{
  return NULL;
}

