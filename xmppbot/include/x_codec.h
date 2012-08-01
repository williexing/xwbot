/*
 * x_codec.h
 *
 *  Created on: 26.10.2010
 *      Author: artur
 */

#ifndef X_CODEC_H_
#define X_CODEC_H_

#include <speex/speex.h>

int x_speex_encdata (void *state,SpeexBits *bits, float *data,
		int datasiz, char *result, int *len);
void *x_speex_default_state(SpeexBits *bits);
void x_speex_delete_state(void *state, SpeexBits *bits);

#endif /* X_CODEC_C_ */
