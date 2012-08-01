/*
 * x_ref.c
 *
 *  Created on: 06.04.2011
 *      Author: artur
 */

#include "x_lib.h"

#include "x_ref.h"

/**
 *  qq
 */
EXPORT_SYMBOL void
xref_init(struct xref *ref)
{
  ref->counter = 1;
}

/**
 * qq
 */
EXPORT_SYMBOL void
xref_put(struct xref *ref, void
(*fn)(struct xref *))
{
  if (ref && --ref->counter < 0 && fn)
    fn(ref);
}

/**
 * qq
 */
EXPORT_SYMBOL struct xref *
xref_get(struct xref *ref)
{
  if (ref) ++ref->counter;
  return ref;
}
