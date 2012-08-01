/*
 * x_vp8.h
 *
 *  Created on: Oct 25, 2011
 *      Author: artemka
 */

#ifndef X_VP8_H_
#define X_VP8_H_

#include <sys/types.h>

#include <vpx/vpx_encoder.h>
#include <vpx/vpx_decoder.h>

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
  // PictureID is considered to be absent if |picture_id| is set to kuint32max.
  u_int32_t picture_id;
} vp8_descriptor_t;

int get_vp8_descriptor_size(const struct vp8_descriptor *descriptor);
void pack_vp8_descriptor(const struct vp8_descriptor *descriptor, u_int8_t * buffer,
    int buffer_size);
int parse_vp8_packet(const unsigned char * buffer, int buffer_size,
    struct vp8_descriptor* descriptor);
void on_vp8_packet_received(void *data, int len, void *cb_data);
int x_vpx_decoder_init(vpx_dec_ctx_t *_decoder, int numcores);
void x_vp8_init(void);

#endif /* X_VP8_H_ */
