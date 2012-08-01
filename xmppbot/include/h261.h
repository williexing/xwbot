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
 * CXmppAgent
 * Created on: 20.08.2010
 *     Author: artur
 *
 */

#ifndef H261_H_
#define H261_H_

#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

/**
 * h.261 packet header
 */
typedef struct h_261_phdr
{
	uint32_t vmvd :5;
	uint32_t hmvd :5;
	uint32_t quant :5;
	uint32_t mbap :5;
	uint32_t gobn :4;
	uint32_t v :1;
	uint32_t i :1;
	uint32_t ebit :3;
	uint32_t sbit :1;
	uint8_t stream[];
} h_261_phdr_t;

int h261_init_decoder(char *fname);
int h261_init_encoder(void);
AVPacket *h261_encode(void);

#endif /* H261_H_ */
