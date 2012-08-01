/*
 * xwxxtest.cpp
 *
 *  Created on: Nov 2, 2011
 *      Author: artemka
 */

#include <string.h>
#include <stdio.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[xw-objc] "

#include <xwlib/x_obj.h>

#import "x_objc.h"

static const char *ns1 = "__testobj";
static const char *nam1 = "gobee";

int
main(int argc, char **argv)
{
  XObject *__xobj;
  XObject1 *__xobj1;

  printf("Allocating object (%p,%p)\n",ns1,nam1);
  
  __xobj1 = [XObject1 alloc: ns1
  ns: nam1];

  [((XObject *)__xobj1) foo];
  [__xobj1 foo];

//  printf("sizeof(XObject)=%d,%d\n", sizeof(XObject),__alignof__(XObject));
//  printf("sizeof(XObject1)=%d\n",sizeof(XObject1),__alignof__(XObject1));
  getchar();

}
