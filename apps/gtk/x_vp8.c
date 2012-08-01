/*
 * x_vp8.c
 *
 *  Created on: Oct 25, 2011
 *      Author: artemka
 */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "mynet.h"
#include "x_vp8.h"

static inline u_int8_t
extract_bits(u_int8_t byte, int bits_count, int shift)
{
  return (byte >> shift) & ((1 << bits_count) - 1);
}

static vpx_dec_ctx_t _decoder;

void
x_vp8_init(void)
{
  memset(&_decoder, 0, sizeof(_decoder));
  x_vpx_decoder_init(&_decoder, 1);
}

// VP8 Payload Descriptor format:
//
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | RSV |I|N|FI |B|         PictureID (integer #bytes)            |
// +-+-+-+-+-+-+-+-+                                               |
// :                                                               :
// |               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |               : (VP8 data or VP8 payload header; byte aligned)|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// On the diagram above order of bytes and order of bits within each
// byte are big-endian. So bits 0 and 7 are the most and the least
// significant bits in the first byte, bit 8 is the most significant
// bit in the second byte, etc.
//
// RSV: 3 bits
// Bits reserved for future use. MUST be equal to zero and MUST be
// ignored by the receiver.
//
// I: 1 bit
// PictureID present. When set to one, a PictureID is provided after
// the first byte of the payload descriptor. When set to zero, the
// PictureID is omitted, and the one-byte payload descriptor is
// immediately followed by the VP8 payload.
//
// N: 1 bit
// Non-reference frame. When set to one, the frame can be discarded
// without affecting any other future or past frames.
//
// FI: 2 bits
// Fragmentation information field. This field contains information
// about the fragmentation of VP8 payloads carried in the RTP
// packet. The four different values are listed below.
//
// FI   Fragmentation status
//   00 The RTP packet contains no fragmented VP8 partitions. The
//      payload is one or several complete partitions.
//   01 The RTP packet contains the first part of a fragmented
//      partition. The fragment must be placed in its own RTP packet.
//   10 The RTP packet contains a fragment that is neither the first nor
//      the last part of a fragmented partition. The fragment must be
//      placed in its own RTP packet.
//   11 The RTP packet contains the last part of a fragmented
//      partition. The fragment must be placed in its own RTP packet.
//
// B: 1 bit
// Beginning VP8 frame. When set to 1 this signals that a new VP8
// frame starts in this RTP packet.
//
// PictureID: Multiple of 8 bits
// This is a running index of the frames. The field is present only if
// the I bit is equal to one. The most significant bit of each byte is
// an extension flag. The 7 following bits carry (parts of) the
// PictureID. If the extension flag is one, the PictureID continues in
// the next byte. If the extension flag is zero, the 7 remaining bits
// are the last (and least significant) bits in the PictureID. The
// sender may choose any number of bytes for the PictureID. The
// PictureID SHALL start on a random number, and SHALL wrap after
// reaching the maximum ID as chosen by the application

int
get_vp8_descriptor_size(const struct vp8_descriptor *descriptor)
{
  if (descriptor->picture_id == __UINT32_MAX__)
    return 1;
  int result = 2;
  // We need 1 byte per each 7 bits in picture_id.
  u_int32_t picture_id = descriptor->picture_id >> 7;
  while (picture_id > 0)
    {
      picture_id >>= 7;
      ++result;
    }
  return result;
}

void
pack_vp8_descriptor(const struct vp8_descriptor *descriptor, u_int8_t * buffer,
    int buffer_size)
{

  if (buffer_size <= 0)
    return;

  buffer[0] = ((u_int8_t) (descriptor->picture_id != __UINT32_MAX__) << 4)
      | ((u_int8_t) descriptor->non_reference_frame << 3)
      | (descriptor->fragmentation_info << 1)
      | ((u_int8_t) descriptor->frame_beginning);

  u_int32_t picture_id = descriptor->picture_id;
  if (picture_id == __UINT32_MAX__)
    return;

  int pos = 1;
  while (picture_id > 0)
    {
      if (pos >= buffer_size)
        break;
      buffer[pos] = picture_id & 0x7F;
      picture_id >>= 7;

      // Set the extension bit if neccessary.
      if (picture_id > 0)
        buffer[pos] |= 0x80;
      ++pos;
    }
}

int
parse_vp8_packet(const unsigned char * buffer, int buffer_size,
    struct vp8_descriptor* descriptor)
{

  if (buffer_size <= 0)
    return (-1);

  u_int8_t picture_id_present = extract_bits(buffer[0], 1, 4) != 0;
  descriptor->non_reference_frame = extract_bits(buffer[0], 1, 3) != 0;
  descriptor->fragmentation_info = extract_bits(buffer[0], 2, 1);
  descriptor->frame_beginning = extract_bits(buffer[0], 1, 0) != 0;

  // Return here if we don't need to decode PictureID.
  if (!picture_id_present)
    {
      descriptor->picture_id = __UINT32_MAX__;
      return 1;
    }

  // Decode PictureID.
  u_int8_t extension = 1;
  int pos = 1;
  descriptor->picture_id = 0;
  while (extension)
    {
      if (pos >= buffer_size)
        return -1;

      descriptor->picture_id |= buffer[pos] & 0x7F;
      extension = (buffer[pos] & 0x80) != 0;
      pos += 1;
    }
  return pos;
}

void
on_vp8_packet_received(void *data, int len, void *cb_data)
{
  void *packet_frag;
  int off;
  int psiz = len;
  vp8_descriptor_t vp8d;
  rtp_hdr_t *_rtp;

  if (!data)
    return;

  _rtp = (rtp_hdr_t *) data;
  psiz -= sizeof(rtp_hdr_t);
  off = parse_vp8_packet(_rtp->payload, psiz, &vp8d);

  if (off <= 0)
    return;

  switch (vp8d.fragmentation_info)
    {
  case NOT_FRAGMENTED:
    printf("Not fragmented!");
    break;
  case FIRST_FRAGMENT:
    printf("First frag!");
    break;
  case MIDDLE_FRAGMENT:
    printf("Middle frag!");
    break;
  case LAST_FRAGMENT:
    printf("Last frag!");
    break;
    };

  printf(": %s %s, PIC-ID=%u\n", vp8d.frame_beginning ? "BEGINNING" : "",
      vp8d.non_reference_frame ? "REF" : "NO-REF",
      vp8d.frame_beginning ? vp8d.picture_id : 0);

  packet_frag = _rtp->payload + off;
  psiz -= off;
  return;
}

