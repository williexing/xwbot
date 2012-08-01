/*
 * x_obj.cpp
 *
 *  Created on: Nov 2, 2011
 *      Author: artemka
 */
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <stdarg.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[xwlib++] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include "x_objc.h"

@implementation XObject
+ alloc: (const char *) cname ns: (const char *) namespace
  {
    printf("%s()\n",__FUNCTION__);
    return (id)(void *)x_object_new(cname);
  }
- foo
  {
    printf("%s()\n",__FUNCTION__);
  }
@end

@implementation XObject1
- foo
  {
    printf("%s()\n",__FUNCTION__);
  }
@end

typedef void*
(*IMP)(void *, SEL, ...);

typedef const struct objc_selector
{
  void *sel_id;
  const char *sel_types;
}*SEL;

/* Whereas a Module (defined further down) is the root (typically) of a file,
 a Symtab is the root of the class and category definitions within the
 module.  
 
 A Symtab contains a variable length array of pointers to classes and
 categories  defined in the module.   */
typedef struct objc_symtab
{
  unsigned long sel_ref_cnt; /* Unknown. */
  SEL refs; /* Unknown. */
  unsigned short cls_def_cnt; /* Number of classes compiled
   (defined) in the module. */
  unsigned short cat_def_cnt; /* Number of categories 
   compiled (defined) in the 
   module. */

  void *defs[1]; /* Variable array of pointers.
   cls_def_cnt of type Class 
   followed by cat_def_cnt of
   type Category_t, followed
   by a NULL terminated array
   of objc_static_instances. */
} Symtab, *Symtab_t;

/*
 ** The compiler generates one of these structures for each module that
 ** composes the executable (eg main.m).  
 ** 
 ** This data structure is the root of the definition tree for the module.  
 ** 
 ** A collect program runs between ld stages and creates a ObjC ctor array. 
 ** That array holds a pointer to each module structure of the executable. 
 */
typedef struct objc_module
{
  unsigned long version; /* Compiler revision. */
  unsigned long size; /* sizeof(Module). */
  const char* name; /* Name of the file where the 
   module was generated.   The 
   name includes the path. */

  Symtab_t symtab; /* Pointer to the Symtab of
   the module.  The Symtab
   holds an array of 
   pointers to 
   the classes and categories 
   defined in the module. */
} Module, *Module_t;

void *
___objc_msg_forward(void *obj, SEL op, ...)
{
  va_list ap;
  int i, sum;
  unsigned long long v;

  printf("%s(): ", __FUNCTION__);
  printf("%p;", obj);

  va_start(ap, op); /* Initialize the argument list. */

    for (i = 0; i < 3; i++)
      {
        v = va_arg (ap, unsigned long long);
        fflush(stdout);
        if (i<3)
          printf("'%s'",(const char *)v);
    }

    va_end (ap); /* Clean up. */
    printf("\n");
    return NULL;
}

void *
objc_lookup_class(const char *name)
{
  void *c;
  c = (void *) x_object_new(name);
  printf("%s(), c(%p)\n", __FUNCTION__,c);
  return c;
}

IMP
objc_msg_lookup(id receiver, SEL op)
{
  printf("%s(),%p:'%s'\n", __FUNCTION__, op->sel_id, op->sel_types);
  return &___objc_msg_forward;
}

void
__objc_exec_class(Module_t module)
{
  printf("%s()\n", __FUNCTION__);
  printf("Executing module of size %d from '%s'\n", module->size, module->name);
}

