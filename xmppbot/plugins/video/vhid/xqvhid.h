/**
 * xqvfb.h
 *
 *  Created on: Jul 4, 2012
 *      Author: artemka
 */

#ifndef XQVFB_H_
#define XQVFB_H_

struct QVFbKeyData
{
    unsigned int keycode;
    unsigned int modifiers;
    unsigned short int unicode;
    int press; // sizeof bool
    int repeat; // sizeof bool
};

enum
{
    HID_MOUSE_EVENT = 0x1,
    HID_KEY_EVENT,
    HID_SCROLL_EVENT,
};

#endif /* XQVFB_H_ */
