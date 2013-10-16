/*
 * x_theora.h
 *
 *  Created on: Oct 26, 2011
 *      Author: artemka
 */

#ifndef X_THEORA_H_
#define X_THEORA_H_

//#include <theora/theora.h>
#include <theora/theoradec.h>
#include <theora/theoraenc.h>

#include <xwlib/x_types.h>

enum
{
  TYPE_UNSPECIFIED = -1,
  TYPE_ENCODER = 1,
  TYPE_DECODER = 2,
};

typedef struct theora_packed_header
{
  uint32_t ident;
  uint16_t plen __attribute__((packed));
  char data[];
} theora_packed_header_t __attribute__((packed));

#define THEORA_HDR_SIZE 6

typedef struct x_theora_context
{
  int ident;
  unsigned int status;
  th_info tinfo;

  int type;

  union
  {
    th_enc_ctx *ed;
    th_dec_ctx *dd;
  } codec;

  th_comment tcmnt;
  th_setup_info *tsinfo;
  th_ycbcr_buffer lastframe;
  theora_packed_header_t *header;
  uint32_t header_len;

  char *buffer;
  int bufsiz;
} x_theora_context_t;

void x_theora_encoder_init(x_object *obj, x_theora_context_t *cdc);
void x_theora_decoder_init(x_theora_context_t *cdc);
void x_theora_context_init(x_object *obj, x_theora_context_t *cdc);
void __on_theora_packet_received(x_theora_context_t *cdc, void *_data, int len);
void x_theora_context_resize(int w, int h, int _stride, int sock, struct sockaddr *addr,
    int addrlen);
void theora_read_str_config(x_theora_context_t *cdc, const char *confstr);

#endif /* X_THEORA_H_ */
