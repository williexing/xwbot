/*
 * x_theora.h
 *
 *  Created on: Oct 26, 2011
 *      Author: artemka
 */

#ifndef X_THEORA_H_
#define X_THEORA_H_

#include <theora/theora.h>
#include <theora/theoradec.h>
#include <theora/theoraenc.h>

typedef struct x_theora_codec
{
  int ident;
  unsigned int status;
  th_info tinfo;
  th_dec_ctx *tctx;
  th_comment tcmnt;
  th_setup_info *tsinfo;
} x_theora_codec_t;

void x_theora_init(x_theora_codec_t *cdc);
void on_theora_packet_received(void *data, int len, void *cb_data);
void gobee_theora_push_frame(void *frame, int sock, struct sockaddr *addr,
    int addrlen);
void gobee_theora_resize(int w, int h, int _stride, int sock, struct sockaddr *addr,
    int addrlen);

#endif /* X_THEORA_H_ */
