/*
 * x_vp8.h
 *
 *  Created on: Oct 25, 2011
 *      Author: artemka
 */

#ifndef X_VP8_H_
#define X_VP8_H_

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>

#include <vpx/vpx_encoder.h>
#include <vpx/vpx_decoder.h>
//#include <vpx/vp8dx.h>
//#include <vpx/vp8cx.h>

#ifdef _WIN32
#       include <memory.h>
  typedef unsigned __int8 u_int8_t;
  typedef unsigned __int32 u_int32_t;

#       ifndef EBADFD
#       	define EBADFD EBADF
#       endif
#else
#       include <arpa/inet.h>
#       include <unistd.h>
#endif /* WIN32 */

#       ifndef __UINT32_MAX__
#             define __UINT32_MAX__ (0L-1)
#       endif

#ifdef __cplusplus
extern "C" {
#endif

enum frag_info
{
  NOT_FRAGMENTED = 0,
  FIRST_FRAGMENT = 1,
  MIDDLE_FRAGMENT = 2,
  LAST_FRAGMENT = 3,
};

// vp8_descriptor struct used to store values of the VP8 RTP descriptor
// fields. Meaning of each field is documented in the RTP Payload
// Format for VP8 spec: http://www.webmproject.org/code/specs/rtp/ .
typedef struct vp8_descriptor
{
  u_int8_t non_reference_frame;
  u_int8_t fragmentation_info;
  u_int8_t frame_beginning;
  u_int32_t picture_id;
} vp8_descriptor_t;

EXPORT_SYMBOL int get_vp8_descriptor_size(const struct vp8_descriptor *descriptor);
EXPORT_SYMBOL int pack_vp8_descriptor(const struct vp8_descriptor *descriptor, u_int8_t * buffer, int buffer_size);
EXPORT_SYMBOL int parse_vp8_packet(const unsigned char * buffer, int buffer_size, struct vp8_descriptor *descriptor);
EXPORT_SYMBOL void on_vp8_packet_received(void *data, int len, void *cb_data);
EXPORT_SYMBOL int x_vpx_decoder_init(vpx_dec_ctx_t *_decoder, int numcores);
EXPORT_SYMBOL void x_vp8_init(void);
EXPORT_SYMBOL void x_vpx_enc_run(unsigned char *pixels, int w, int h, int sock, struct sockaddr_in *cli);
EXPORT_SYMBOL int x_vpx_encoder_init(vpx_codec_ctx_t *_p_encoder, int numcores, int width, int height);

#define VP8_PTYPE 45

#ifdef __cplusplus
};
#endif

#endif /* X_VP8_H_ */
