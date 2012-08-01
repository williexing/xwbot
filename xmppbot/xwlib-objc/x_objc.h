/*
 * x_obj.hpp
 *
 *  Created on: Nov 2, 2011
 *      Author: artemka
 */

#ifndef X_OBJ_HPP_
#define X_OBJ_HPP_

#include <sys/types.h>
#include <xwlib/x_obj.h>

#ifdef __cplusplus
extern "C"
  {
#endif

/*
 * All classes are derived from XObject.
 */
@interface XObject
  {
    x_object xobj;
  }

+ alloc: (const char *)cname ns: (const char *) namespace;
- foo;

@end

@interface XObject1 : XObject
  {
    // uint32_t xid;
  unsigned short s;
  unsigned short s2;
  unsigned char s3;
  unsigned int s4;
  }
- foo;
@end

#ifdef __cplusplus
}
#endif

#endif /* X_OBJ_HPP_ */
