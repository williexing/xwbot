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

  unsigned short seq; // sequence number
  unsigned int t_stamp; // timestamp
  unsigned int ssrc_id; // SSRC ID
  // unsigned int csrc_id;      // SSRC ID
  unsigned char payload[];
} rtp_hdr_t;

typedef struct theora_packed_header
{
  unsigned int ident;
  unsigned short plen;
  char data[];
} theora_packed_header_t;

#define VP8_PTYPE 45
#define THEORA_PTYPE 96
#define POINTER_PTYPE 123

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

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480

#include <arpa/inet.h>

int v4l2_capture_init(unsigned char **, int *w, int *h);
int v4l2_next_frame(int fd);
void nv21_to_i420p(unsigned char *chromaFROM, unsigned char *chromaTO, int w, int h);

int rtp_send(int fd, const void *buf, int count, struct sockaddr *saddr,
    int addrlen, unsigned int t_stamp, int ptype, int seqno);

#endif /* MYNET_H_ */
