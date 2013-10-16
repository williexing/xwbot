/**
 * xqvfb.h
 *
 *  Created on: Jul 4, 2012
 *      Author: artemka
 */

#ifndef XQVFB_H_
#define XQVFB_H_

#include <xwlib/x_utils.h>

#ifndef QT_QWS_TEMP_DIR
#define QT_QWS_TEMP_DIR "/tmp"
#endif

#define MAX_PIPE_NAMELEN 256

#define QT_VFB_DATADIR          "/tmp/qtembedded-%d"
#define QT_VFB_MOUSE_PIPE       "/tmp/.qtvfb_mouse-%d"
#define QT_VFB_KEYBOARD_PIPE    "/tmp/.qtvfb_keyboard-%d"
#define QT_VFB_MAP              "/tmp/.qtvfb_map-%d"
#define QT_VFB_SOUND_PIPE       "/tmp/.qt_soundserver-%d"
#define QTE_PIPE                "/tmp/qtembedded-%d/QtEmbedded-%d"
#define QTE_PIPE_QVFB           "/tmp/qtembedded-%d/QtEmbedded-%d"

#define QVFB_GENID_TRY_MAX 3 /* try 3 times to find unused display Id */

struct QVFbHeader
{
    int width;
    int height;
    int depth;
    int linestep;
    int dataoffset;
    int update_x1;
    int update_y1;
    int update_x2;
    int update_y2;
    int dirty; // TODO really dirty defined as 'bool' type
    int  numcols;
    unsigned int clut[256]; // QRgb type array
    int viewerVersion;
    int serverVersion;
    int brightness; // since 4.4.0
    unsigned long windowId; // WId type since 4.5.0
};

struct QVFbKeyData
{
    unsigned int keycode;
    unsigned int modifiers;
    unsigned short int unicode;
    int press; // sizeof bool
    int repeat; // sizeof bool
};

typedef struct _QVFbSession
{
    int dId;
    int mousePipe;
    int kbdPipe;
    struct QVFbHeader *displayMem;
} QVFbSession;

QVFbSession *qvfb_create(KEY sid, int w, int h, int depth);

#endif /* XQVFB_H_ */
