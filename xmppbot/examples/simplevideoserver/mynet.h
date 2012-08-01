/*
 * mynet.h
 *
 *  Created on: Jul 8, 2011
 *      Author: artemka
 */

#ifndef MYNET_H_
#define MYNET_H_

#include <sys/types.h>

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

  uint16_t seq; // sequence number
  uint32_t t_stamp; // timestamp
  uint32_t ssrc_id; // SSRC ID
  // unsigned int csrc_id;      // SSRC ID
  char payload[];
} rtp_hdr_t;

typedef struct theora_packed_header
{
  uint32_t ident;
  uint16_t plen;
  char data[];
} theora_packed_header_t;


#define WIN_EVENT_PTYPE 123
typedef struct pointer_data
  {
  uint32_t control;
  uint32_t x;
  uint32_t y;
  } pointer_data_t;

#define HOST "127.0.0.1"
#define SRVHOST "127.0.0.1"
#define PORT 2222
#define SRVPORT 62222

#define VIDEO_WIDTH 320
#define VIDEO_HEIGHT 272

#endif /* MYNET_H_ */
