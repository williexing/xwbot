/*
 * mynet.h
 *
 *  Created on: Jul 8, 2011
 *      Author: artemka
 */

#ifndef MYNET_H_
#define MYNET_H_

#define __LE__
/**
 * RTP header
 */
typedef struct rtp_hdr
{
  union
  {
    struct
    {
#ifdef  __LE__
      unsigned char cc :4; // count of contributing sources
      unsigned char x :1; // extensions
      unsigned char p :1; // padding
      unsigned char v :2; // version
#else
    unsigned char v:2; // version
    unsigned char p:1; // padding
    unsigned char x:1; // extensions
    unsigned char cc:4; // count of contributing sources
#endif
    } control1;
    unsigned char raw;
  } octet1;

  union
  {
    struct
    {
#ifdef  __LE__
      unsigned char pt :7; // payload type
      unsigned char m :1; // marker
#else
    unsigned char m:1; // marker
    unsigned char pt:7; // payload type
#endif
    } control2;
    unsigned char raw;
  } octet2;

  unsigned short seq; /* sequence number */
  unsigned int t_stamp; /* timestamp */
  unsigned int ssrc_id; /* SSRC ID */
  // unsigned int csrc_id; /* SSRC ID */
  char payload[];
} rtp_hdr_t;

#define WIN_EVENT_PTYPE 123
typedef struct pointer_data
{
  unsigned int control;
  unsigned int x;
  unsigned int y;
} pointer_data_t;

#define LOCAL_IP "127.0.0.1"
#define REMOTE_IP "127.0.0.1"
#define LOCAL_PORT 2222
#define REMOTE_PORT 62222

#define VIDEO_WIDTH 320
#define VIDEO_HEIGHT 272

typedef void ( udp_callback_fn_t)(void *data, int len, void *cb_data);
int udp_listen(const char *host, int port, udp_callback_fn_t *cb_func, void *cb_data);

#define MTU 1300

#endif /* MYNET_H_ */
