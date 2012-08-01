/*
 * Copyright (c) 2010, artur
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * rtp.h
 *
 *  Created on: Aug 19, 2010
 *      Author: artur
 */

#ifndef RTP_H_
#define RTP_H_

#include <xwlib/x_types.h>
/* #include <xmppagent.h> */


enum {
	UNKNOWN = 0,
	OPTIONS,
	DESCRIBE,
	SETUP,
	PLAY,
	PAUSE,
	TEARDOWN,
} ;

enum
{
	REQUEST,
	RESPONSE,
};

typedef struct rtsp_request
{
	int type;
	int m_id;
	int errcode;
	int seq;
	char *uri;
    char **attr;     // tag attributes { name, value, name, value, ... NULL }
    void *payload;      // tag character content, empty string if none
} rtsp_request_t;


#ifndef __LE__
#define __LE__
#endif

/**
 * RTP header
 */
typedef struct rtp_hdr
{
		union {
			struct {
#ifdef	__LE__
				unsigned char cc:4;	// count of contributing sources
				unsigned char x:1;	// extensions
				unsigned char p:1;	// padding
				unsigned char v:2;	// version
#else
				unsigned char v:2;	// version
				unsigned char p:1;	// padding
				unsigned char x:1;	// extensions
				unsigned char cc:4;	// count of contributing sources
#endif
			} control1;
			unsigned char raw;
		} octet1;

		union {
			struct {
#ifdef	__LE__
				unsigned char pt:7;	// payload type
				unsigned char m:1;	// marker
#else
				unsigned char m:1;	// marker
				unsigned char pt:7;	// payload type
#endif
				} control2;
			unsigned char raw;
		} octet2;

		unsigned short seq;	// sequence number
		unsigned int t_stamp;	// timestamp
		unsigned int ssrc_id;	// SSRC ID
		// unsigned int csrc_id;	// SSRC ID
		char payload[];
} rtp_hdr_t;


void *rtp_consumer_tasklet (void *arg);
void *rtp_server(void *);

int rtsp_decode_method(char *str);
int rtsp_req_set_attr (rtsp_request_t *req, char *key, char *val);
char *rtsp_req_get_attr (rtsp_request_t *req, char *key);
rtsp_request_t *rtsp_req_from_str (char *str);
char *rtsp_req_to_str (rtsp_request_t *req);
rtsp_request_t *rtsp_response (rtsp_request_t *req);
rtsp_request_t *rtsp_req_new(int type, int m_id);
void rtsp_req_free (rtsp_request_t *req);

char *sdp_get_descr(char *resource, char *ip);

typedef struct pcap_hdr_s {
  uint32_t magic_number; /* magic number */
  uint16_t version_major; /* major version number */
  uint16_t version_minor; /* minor version number */
  int32_t thiszone; /* GMT to local correction */
  uint32_t sigfigs; /* accuracy of timestamps */
  uint32_t snaplen; /* max length of captured packets, in octets */
  uint32_t network; /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
  uint32_t ts_sec; /* timestamp seconds */
  uint32_t ts_usec; /* timestamp microseconds */
  uint32_t incl_len; /* number of octets of packet saved in file */
  uint32_t orig_len; /* actual length of packet */
} pcaprec_hdr_t;

#define ENDL "\r\n"

#endif /* RTP_H_ */
