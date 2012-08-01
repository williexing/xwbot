/*
 * Copyright (c) 2011-2012, CrossLine Media,Ltd. www.clmedia.ru
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

/**
 * evtdomain.c
 *
 *  Created on: Jan 26, 2012
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[__evtdomain] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <ev.h>

typedef struct xeventdomain
{
  x_object xobj;
  struct
  {
    int state;
    u_int32_t cr0;
  } regs;
  THREAD_T input_tid;
  /** @todo It will be better to embed eventloop (no pointer)!!! */
  struct ev_loop *eventloop;
} x_event_domain;

UNUSED static void *
evtdomain_worker(void *ctx)
{
  x_event_domain *evo = (x_event_domain *) (void *) ctx;

  ENTER;

  do
    {
      /*ev_loop(evo->eventloop, EVLOOP_NONBLOCK);*/
      ev_loop(evo->eventloop, 0);
    }
  while (!(evo->regs.cr0 & (1 << 0)) /* thread stop condition!!! */);

  EXIT;
  return NULL;
}

static BOOL
evtdomain_equals(x_object *o, x_obj_attr_t *attrs)
{
  ENTER;
  EXIT;
  return TRUE;
}

static void
evtdomain_on_create(x_object *o)
{
  x_event_domain *evo = (x_event_domain *) (void *) o;
  ENTER;
  evo->eventloop = ev_loop_new(EVFLAG_AUTO);
  EXIT;
}

static int
evtdomain_append_cb(x_object *o, x_object *parent)
{
  ENTER;
  EXIT;
  return 0;
}

/* handler of removing event (appears when object is removing from its parent) */
static void
evtdomain_remove_cb(x_object *o)
{
  ENTER;
  EXIT;
}

/* default constructor */
static void
evtdomain_release_cb(x_object *o)
{
  ENTER;
  EXIT;
}

/* assignment handler */
static x_object *
evtdomain_on_assign(x_object *o, x_obj_attr_t *attrs)
{
  ENTER;
  EXIT;
  return o;
}


static struct xobjectclass __evtdomain_class;

__x_plugin_visibility __plugin_init void
	__evtdomain_init(void)
{
	__evtdomain_class.cname = "__evtdomain";
	__evtdomain_class.psize = sizeof(x_event_domain) - sizeof(x_object);
	__evtdomain_class.match = &evtdomain_equals;
	__evtdomain_class.on_create = &evtdomain_on_create;
	__evtdomain_class.on_append = &evtdomain_append_cb;
	__evtdomain_class.on_remove = &evtdomain_remove_cb;
	__evtdomain_class.on_release = &evtdomain_release_cb;
	__evtdomain_class.on_assign = &evtdomain_on_assign;

	x_class_register(__evtdomain_class.cname, &__evtdomain_class);
	return;
}

PLUGIN_INIT(__evtdomain_init)
	;
