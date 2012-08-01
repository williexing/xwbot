/*
 * xwxxtest.cpp
 *
 *  Created on: Nov 2, 2011
 *      Author: artemka
 */

#include <string.h>
#include <stdio.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[xwlib++] "

#include "x_objxx.h"

static void
test1(int i)
{
  ::gobee::__x_object *xobj;
  for (; i > 0; i--)
    {
      xobj
          = new ((const char *) "gobee", (const char *) "__testobj") ::gobee::__x_object();
      delete (xobj);
    }
}

static void
test2(int i)
{
  ::gobee::__x_object *root;
  ::gobee::__x_object *xobj;
  ::gobee::__x_object *tmp;

  root
      = new ((const char *) "gobee", (const char *) "__testobj") ::gobee::__x_object();
  xobj = root;
  do
    {
      tmp
          = new ((const char *) "gobee", (const char *) "__testchild") ::gobee::__x_object();
      xobj->add_child(tmp);
      xobj = tmp;
    }
  while (--i > 0);

  delete (root);
}

static int
test3(void)
{
  int ret = 0x0;
  ::gobee::__x_object *xobj;

  ::printf("\n--------\nTESTING CORRECTNESS OF SETTING/GETTING\n"
    "OBJECT ATTRIBUTES\n--------\n\n");

  xobj
      = new ((const char *) "gobee", (const char *) "__testobj") ::gobee::__x_object();
  xobj->__setattr("k1", "hello");
  xobj->__setattr("k2", "world");

  if (!::strcmp((*xobj)["k1"], "hello"))
    ::printf("attribute 1 passed.\n");
  else
    ret |= 0x1;

  if (!::strcmp((*xobj)["k2"], "world"))
    ::printf("attribute 2 passed.\n");
  else
    ret |= 0x2;

  delete (xobj);

  return ret;
}

int
main(int argc, char **argv)
{
  int cnt = ::strtol(argv[1], (char **) NULL, 10);

  test1(cnt);
  printf("ok!");
  getchar();

  test2(cnt);
  printf("ok!");
  getchar();

  if (test3())
    printf("error!");
  else
    printf("ok!");

  getchar();
}
