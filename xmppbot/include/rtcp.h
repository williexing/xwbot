/*
 * rtcp.h
 *
 *  Created on: Aug 24, 2010
 *      Author: artur
 */

#ifndef RTCP_H_
#define RTCP_H_

#define __LE__

#define RTCP_PT_SR	200

typedef struct rtcp_hdr
{
  union
  {
    struct
    {
#ifdef	__LE__
      unsigned char rc :5; // extensions
      unsigned char p :1; // padding
      unsigned char v :2; // version
#else
    unsigned char v:2; // version
    unsigned char p:1; // padding
    unsigned char rc:1; // extensions
#endif
    } control1;
    unsigned char raw;
  } octet1;

  unsigned char pt;
  unsigned short length; // sequence number
  unsigned int ssrc_id; // SSRC ID
  unsigned int ntp_ts_hi;
  unsigned int ntp_ts_lo;
  unsigned int rtp_ts;
  unsigned int p_count;
  unsigned int b_count;
} rtcp_hdr_t;

int
send_RTCP_SR(void);

#endif /* RTCP_H_ */
