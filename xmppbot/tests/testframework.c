/*
 * x_objfactory.c
 *
 *  Created on: Aug 12, 2011
 *      Author: artemka
 */

#include <stdio.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[ofactory] "
#include <x_utils.h>
#include <x_obj.h>

typedef struct test_unit
{
  char *name;
  int (*runtest)(struct test_unit *);
} test_unit;

/** default class map table */
static struct ht_cell *testmap[HTABLESIZE];

EXPORT_SYMBOL x_objectclass *
test_unit_get(const char *name)
{
  int i;
  return (x_objectclass *) ht_get((KEY) name,
      (struct ht_cell **) &testmap, &i);
}

/**
 * Registers new object class
 */
EXPORT_SYMBOL int
test_unit_register(const char *name, test_unit *unit)
{
  TRACE("Registering unit test '%s'\n",unit->name);
  ht_set((KEY) name, (VAL) unit, (struct ht_cell **) &testmap);
  return 0;
}
