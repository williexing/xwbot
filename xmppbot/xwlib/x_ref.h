/*
 * x_ref.h
 *
 *  Created on: 06.04.2011
 *      Author:
 */

#ifndef X_REF_H_
#define X_REF_H_

#include "x_lib.h"

/**
 * Smart reference structure
 *
 * @ingroup XREF
 *
 */
struct xref
{
  int counter;
};

/**
 * @defgroup XREF Xref API
 *
 * @{
 */
EXPORT_SYMBOL void xref_init(struct xref *);
EXPORT_SYMBOL void xref_put(struct xref *, void(*)(struct xref *));
EXPORT_SYMBOL struct xref *xref_get(struct xref *);
/** @} */

#endif /* X_REF_H_ */
