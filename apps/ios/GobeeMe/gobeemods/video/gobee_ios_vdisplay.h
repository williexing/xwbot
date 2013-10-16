//
//  gobee_ios_vdisplay.h
//  GobeeMe
//
//  Created by артемка on 08.03.13.
//
//

#ifndef GobeeMe_gobee_ios_vdisplay_h
#define GobeeMe_gobee_ios_vdisplay_h

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct virtual_display
{
  x_object xobj;
  
  int left_top_x;
  int left_top_y;
  int viewportWidth;
  int viewportHeight;
  
  int frameWidth;
  int frameHeight;
  
  int displayWidth;
  int displayHeight;

  void *UI_ctx; //
  
  img_plane_buffer yuv;
  
} virtual_display_t;

#ifdef __cplusplus
};
#endif

#endif
