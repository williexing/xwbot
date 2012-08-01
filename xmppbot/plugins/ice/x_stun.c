/*
 * Copyright (c) 2010, Artur Emagulov
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
 * Created on: 26.09.2010
 *     Author: Artur Emagulov
 *
 */

#undef DEBUG_PRFX
//#define DEBUG_PRFX "[stun_API] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <crypt/crc32.h>
#include <crypt/md5.h>
#include <crypt/sha1_rfc3174.h>

#include "x_stun.h"

#define IPAD 0x36
#define OPAD 0x5c

static stun_packet_t *
stun_pkt_new(void);
static int
stun_set_attr_d(stun_packet_t *pkt, int atype, int len, char *val);
static stun_packet_t *
stun_pkt_new(void);
void
stun_pkt_free(stun_packet_t *pkt);
static stun_hdr_t *
stun_add_integrity(stun_hdr_t *pkt, const char *key);
static uint32_t
stun_get_fingerprint(stun_hdr_t *pkt);
static stun_hdr_t *
stun_hdr_from_packet(stun_packet_t *pkt);

/**
 *
 */
static char *
jingle_get_hmac_sha1(const char *key, int keylen, char *data, int datalen)
{

  unsigned char k_ipad[65];
  unsigned char k_opad[65];
  SHA1Context sha;
  SHA1Context sha2;
  int i;
  uint8_t Message_Digest[20];
  uint8_t *Message_Digest2;
  md5_state_t state;
  md5_byte_t digest[16];
  md5_byte_t *__key = (md5_byte_t *) key;
  int __keylen = keylen;

  x_memset(digest, 0, 16);

  if (__keylen > 64)
    {
      md5_init(&state);
      md5_append(&state, (const md5_byte_t *) __key, __keylen);
      md5_finish(&state, digest);
      __key = digest;
      __keylen = 16;
    }

  x_memset(k_ipad, 0, sizeof(k_ipad));
  x_memset(k_opad, 0, sizeof(k_opad));
  x_memcpy(k_ipad, __key, __keylen);
  x_memcpy(k_opad, __key, __keylen);

  /* XOR key with ipad and opad values */
  for (i = 0; i < 64; i++)
    {
      k_ipad[i] ^= IPAD;
      k_opad[i] ^= OPAD;
    }

  SHA1Reset(&sha);
  SHA1Input(&sha, (const uint8_t *) k_ipad, 64);
  SHA1Input(&sha, (const uint8_t *) data, datalen);
  SHA1Result(&sha, Message_Digest);

  Message_Digest2 = x_malloc(20);
  SHA1Reset(&sha2);
  SHA1Input(&sha2, (const uint8_t *) k_opad, 64);
  SHA1Input(&sha2, (const uint8_t *) Message_Digest, 20);
  SHA1Result(&sha2, Message_Digest2);

  // TRACE("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
  //TRACE("HMAC-SHA1: ");
  // hexdump((char *)Message_Digest2,20);
  //TRACE("\n");

  return (char *) Message_Digest2;
}

/**
 * returns x-mapped address buffer
 */
static char *
stun_x_mapped(struct sockaddr_in *saddr)
{
  uint16_t port;
  uint32_t ip;
  char *xmapped;

  ENTER;

  xmapped = (char *) x_memset(x_malloc(8), 0, 8);

  xmapped[0] = 0;
  xmapped[1] = 1; // ipv4

  TRACE("MAPPING %s:%d\n", inet_ntoa(saddr->sin_addr), ntohs(saddr->sin_port));

  ip = ntohl(saddr->sin_addr.s_addr);
  port = ntohs(saddr->sin_port);
  port ^= 0x2112;
  ip ^= STUN_M_COOKIE;

  port = htons(port);
  ip = htonl(ip);

  x_memcpy(xmapped + 2, &port, 2);
  x_memcpy(xmapped + 4, &ip, 4);

  EXIT;
  return xmapped;
}

/**
 *
 */
stun_hdr_t *
stun_get_response(const char *stun_key, stun_hdr_t *req, struct sockaddr *saddr)
{
  int i;
  char *ptr = 0;
  uint32_t crc;
  stun_packet_t *pkt, *pkt2;
  stun_hdr_t *resp = NULL;
  struct sockaddr_in _sai;

  ENTER;

  if (!stun_key || !req || !saddr)
    return NULL;

  pkt2 = stun_packet_from_hdr(req, stun_key);
  if (!pkt2)
    goto _ret_free_3;
  pkt = stun_pkt_new();
  if (!pkt)
    goto _ret_free_2;

  x_memcpy(&_sai, saddr, sizeof(_sai));

  // add xmapped
  // TRACE("\n");
  if ((ptr = stun_x_mapped(&_sai)))
    {
      stun_set_attr_d(pkt, STUN_X_MAPPED_ADDRESS, 8, ptr);
      x_free(ptr);
    }

  // copy username
  i = stun_attr_id(pkt2, STUN_USERNAME);
  if (i >= 0)
    {
      TRACE("STUN PACKET ADDING ATTRIBUTE: [%s] %d bytes\n",
          pkt2->attrs[i].data, pkt2->attrs[i].len);
      stun_set_attr_d(pkt, STUN_USERNAME, pkt2->attrs[i].len,
          pkt2->attrs[i].data);
    }

  //TRACE("++++++++++=== > stun_hdr_from_packet()\n");
  resp = stun_hdr_from_packet(pkt);
  //TRACE("++++++++++=== < stun_hdr_from_packet()\n");

  resp->mcookie = STUN_NET_M_COOKIE;
  resp->ver_type.raw = htons(STUN_RESP);

  // copy transction id
  x_memcpy(resp->trans_id, req->trans_id, 12);
  resp = stun_add_integrity(resp, stun_key);

  // TRACE("REPLYING STUN2 HEADER: ");
  i = ntohs(resp->mlen);
  // hexdump((char *)resp,i + 20);
  // TRACE("\n");
  // TRACE("STUN2 MLEN=%d\n",i);
  // _stun_packet_printf(pkt);

  // add CRC32
  i = ntohs(resp->mlen) + sizeof(stun_hdr_t);
  resp = x_realloc(resp, i + 8);
  resp->mlen = htons(i + 8 - sizeof(stun_hdr_t));

  crc = stun_get_fingerprint(resp);

  ptr = (char *) resp + i;
  *(uint16_t *) ptr = htons(STUN_FINGERPRINT);
  ptr += sizeof(uint16_t);
  *(uint16_t *) ptr = htons(4);
  ptr += sizeof(uint16_t);
  *(uint32_t *) ptr = htonl(crc ^ STUN_FINGERPRINT_XOR);

  if (pkt)
    stun_pkt_free(pkt);
  _ret_free_2: if (pkt2)
    stun_pkt_free(pkt2);
  _ret_free_3:

  EXIT;
  return resp;
}

/**
 * set attribute of type to malloc'ed value copy of val.
 */
static int
stun_set_attr_d(stun_packet_t *pkt, int atype, int len, char *val)
{
  int i;

  i = stun_attr_id(pkt, atype);
  //TRACE("))))))))))))))))) 2 i=%d, attrnum(%d)\n",i,(int)pkt->attrnum);

  if (i < 0)
    {
      pkt->attrs = x_realloc(pkt->attrs,
          sizeof(stun_attr_t) * (pkt->attrnum + 2));
      i = pkt->attrnum;
    }

  //TRACE("))))))))))))))))) 3\n");

  pkt->attrs[i].type = (uint16_t) atype;
  pkt->attrs[i].len = (uint16_t) len;
  if (pkt->attrs[pkt->attrnum].len)
    {
      //      TRACE("))))))))))))))))) 4\n");

      pkt->attrs[i].data = x_malloc(pkt->attrs[i].len);
      x_memcpy(pkt->attrs[pkt->attrnum].data, val, len);
    }
  else
    pkt->attrs[i].data = NULL;
  // end with NULL
  pkt->attrs[++pkt->attrnum].type = 0;

  // TRACE("))))))))))))))))) 5\n");

  return 0;
}

static stun_packet_t *
stun_pkt_new(void)
{
  stun_packet_t *pkt = NULL;

  ENTER;

  pkt = x_malloc(sizeof(stun_packet_t));
  x_memset(pkt, 0, sizeof(stun_packet_t));

  EXIT;
  return pkt;
}

/**
 *
 */
void
stun_pkt_free(stun_packet_t *pkt)
{
  stun_attr_t *attrs;
  int i;

  ENTER;

  for (attrs = pkt->attrs, i = 0; i < pkt->attrnum; i++)
    {
      if (attrs[i].data)
        x_free(attrs[i].data);
    }
  if (pkt->attrs)
    x_free(pkt->attrs);
  x_free(pkt);

  EXIT;
}

/**
 *
 */
static stun_hdr_t *
stun_add_integrity(stun_hdr_t *pkt, const char *key)
{
  stun_hdr_t *hdr;
  char *ptr;
  char *msg_integrity;
  uint16_t mlen;
  char tdata[128];

  ENTER;

  mlen = ntohs(pkt->mlen) + sizeof(stun_hdr_t);
  // TRACE("----------------------- %d (%d,%d) \n", mlen,ntohs(pkt->mlen), sizeof(stun_packet_t));
  // add message integrity attribute
  hdr = x_realloc(pkt, mlen + 24);

  ptr = ((char *) hdr) + mlen;
  *(uint16_t*) ptr = htons(STUN_MSG_INTEGRITY);
  ptr += sizeof(uint16_t);
  *(uint16_t*) ptr = htons(20);
  ptr += sizeof(uint16_t);

  //TRACE("----------------------- %d\n",mlen);
  mlen += 24;
  //TRACE("----------------------- %d\n",mlen);
  hdr->mlen = htons(mlen - sizeof(stun_hdr_t));
  //TRACE("----------------------- %d\n",ntohs(hdr->mlen));

  x_memset(tdata, 0, sizeof(tdata));
  x_memcpy(tdata, hdr, mlen - 24);

  // Calc Message integrity
  msg_integrity = jingle_get_hmac_sha1(key, strlen(key), (char *) tdata,
      mlen - 24);

  x_memcpy(ptr, msg_integrity, 20);

  if (msg_integrity)
    x_free(msg_integrity);

  EXIT;
  return hdr;
}

/**
 *
 */
static uint32_t
stun_get_fingerprint(stun_hdr_t *pkt)
{
  uint32_t crc;
  uint16_t mlen;
  unsigned char tdata[128];

  ENTER;

  mlen = ntohs(pkt->mlen) + sizeof(stun_hdr_t);
  x_memset(tdata, 0, sizeof(tdata));
  x_memcpy(tdata, pkt, mlen - 8);
  crc = chksum_crc32(tdata, mlen - 8);

  EXIT;
  return crc;
}

/**
 * set attribute of type to x_malloc'ed value copy of val.
 */
static stun_hdr_t *
stun_hdr_from_packet(stun_packet_t *pkt)
{
  int i;
  stun_attr_t *attrs = NULL;
  stun_hdr_t *hdr = NULL;
  char *ptr = NULL;
  uint16_t mlen, alen;

  ENTER;

  mlen = sizeof(stun_hdr_t);
  hdr = x_malloc(mlen);
  x_memset(hdr, 0, mlen);

  for (attrs = pkt->attrs, i = 0; i < pkt->attrnum; i++)
    {
      alen = ADDRLEN_ALIGN(attrs[i].len);

      hdr = x_realloc(hdr, mlen + alen + 2 * sizeof(uint16_t));
      ptr = ((char *) hdr) + mlen;

      // copy type
      *(uint16_t*) ptr = htons(attrs[i].type);
      ptr += sizeof(uint16_t);
      *(uint16_t*) ptr = htons(attrs[i].len);
      ptr += sizeof(uint16_t);
      // copy value
      x_memcpy(ptr, attrs[i].data, attrs[i].len);

      mlen += alen + 2 * sizeof(uint16_t);
      //  TRACE("----------------------> %d\n",mlen);
    }

  TRACE("--------------------->> %d (%d)\n", mlen, mlen - sizeof(stun_hdr_t));
  hdr->mlen = htons(mlen - sizeof(stun_hdr_t));

  EXIT;
  return hdr;
}

/**
 *
 */
stun_hdr_t *
get_stun_simple_request(void)
{
  int i;
  uint32_t crc;
  char *ptr = NULL;

  stun_hdr_t *stn_req = NULL;
  stun_packet_t *stn_pkt = NULL;

  /* send stun request */
  stn_pkt = stun_pkt_new();
  stn_req = stun_hdr_from_packet(stn_pkt);

  stn_req->mcookie = STUN_NET_M_COOKIE;
//  stn_req->mcookie = (int) random();
  *(uint32_t*) &stn_req->trans_id[0] = (int) random();
  *(uint32_t*) &stn_req->trans_id[4] = (int) random();
  *(uint32_t*) &stn_req->trans_id[8] = (int) random();

  stn_req->ver_type.raw = htons(STUN_REQ);

#if 0
  // add CHANGE_REQUEST
  i = ntohs(stn_req->mlen) + sizeof(stun_hdr_t);

  stn_req = x_realloc(stn_req, i + 8);
  stn_req->mlen = htons(i + 8 - sizeof(stun_hdr_t));

  ptr = &((char *) stn_req)[i];
  *(uint16_t *) ptr = htons(STUN_CHANGE_REQUEST);

  ptr += sizeof(uint16_t);
  *(uint16_t *) ptr = htons(4);

  // set change request to change both address and port (value==0x6)
  ptr += sizeof(uint16_t);
  *(uint32_t *) ptr = htonl(0x2);
#else
  i = ntohs(stn_req->mlen) + sizeof(stun_hdr_t);
  stn_req = x_realloc(stn_req, i + 8);
  stn_req->mlen = htons(i + 8 - sizeof(stun_hdr_t));
  crc = stun_get_fingerprint(stn_req);

  ptr = (char *) stn_req + i;
  *(uint16_t *) ptr = htons(STUN_FINGERPRINT);
  ptr += sizeof(uint16_t);
  *(uint16_t *) ptr = htons(4);
  ptr += sizeof(uint16_t);
  *(uint32_t *) ptr = htonl(crc ^ STUN_FINGERPRINT_XOR);
#endif

  stun_pkt_free(stn_pkt);

  TRACE("%p\n", (void *)stn_req);

  return stn_req;
}

EXPORT_SYMBOL stun_hdr_t *
get_stun_full_request(const char *lufrag,
    const char *rufrag, const char *rpwd)
{
  char fullfrag[128];
  int i;
  uint32_t crc;
  char *ptr;
  stun_hdr_t *stn_req = NULL;
  stun_packet_t *stn_pkt = NULL;
  unsigned long int val;

  ENTER;

  TRACE("Generating stun packet for "
  "rufrag(%s),rpwd(%s),lufrag(%s)\n", rufrag, rpwd, lufrag);

  if (!lufrag || !rufrag || !rpwd)
    return NULL;

  sprintf(fullfrag, "%s:%s", rufrag, lufrag);

  stn_pkt = stun_pkt_new();
  stun_set_attr_d(stn_pkt, STUN_PRIORITY, 4, (char *) &stn_pkt);
  val = random();
  stun_set_attr_d(stn_pkt, STUN_ICE_CONTROLLED, 8, (char *) &val);
  stun_set_attr_d(stn_pkt, STUN_USERNAME, strlen(fullfrag), (char *) fullfrag);

  stn_req = stun_hdr_from_packet(stn_pkt);

  stn_req->mcookie = STUN_NET_M_COOKIE;
  stn_req->ver_type.raw = htons(STUN_REQ);
  // set transction id
  x_snprintf(stn_req->trans_id, sizeof(stn_req->trans_id), "%12d",
      (int) random());
  stn_req = stun_add_integrity(stn_req, rpwd);

  // add CRC32
  i = ntohs(stn_req->mlen) + sizeof(stun_hdr_t);
  stn_req = x_realloc(stn_req, i + 8);
  stn_req->mlen = htons(i + 8 - sizeof(stun_hdr_t));
  crc = stun_get_fingerprint(stn_req);

  ptr = (char *) stn_req + i;
  *(uint16_t *) ptr = htons(STUN_FINGERPRINT);
  ptr += sizeof(uint16_t);
  *(uint16_t *) ptr = htons(4);
  ptr += sizeof(uint16_t);
  *(uint32_t *) ptr = htonl(crc ^ STUN_FINGERPRINT_XOR);

  stun_pkt_free(stn_pkt);

  EXIT;
  return stn_req;
}

static void UNUSED
hexdump(char *buf, int len)
{
  int i;
  for (i = 0; i < len; ++i)
    {
      printf("%02x", ((unsigned char) buf[i] & 0xff));
    }
}

stun_packet_t *
stun_packet_from_hdr(stun_hdr_t *req, const char *key)
{
  stun_hdr_t *hdr;
  stun_packet_t *pkt = NULL;
  char *ptr = 0;
  uint16_t tmplen;
  uint32_t crc;
  char *msg_integrity;
  char tdata[128];

  ENTER;

  if (!req)
    return NULL;
  pkt = x_malloc(sizeof(stun_packet_t));

  pkt->hdr = req;
  pkt->len = sizeof(stun_hdr_t) + ntohs(req->mlen);

  TRACE("STUN PACKET: LEN[%d]\n", pkt->len);

  pkt->attrs = x_malloc(sizeof(stun_attr_t) * 1);
  pkt->attrnum = 0;
  pkt->attrs[0].type = 0;

  // parse attributes
  for (ptr = ((char *) req) + sizeof(stun_hdr_t);
      ptr < ((char *) req) + pkt->len;)
    {
      pkt->attrs = x_realloc(pkt->attrs,
          sizeof(stun_hdr_t) * (pkt->attrnum + 2));

      pkt->attrs[pkt->attrnum].type = ntohs(*((uint16_t *) ptr));
      /* check message integrity */
      if (STUN_MSG_INTEGRITY == pkt->attrs[pkt->attrnum].type && key)
        {

          TRACE(
              "Calculating MESSAGE-INTEGRITY with key='%s',keylen(%d),datalen(%d)\n",
              key, (int)strlen(key), (int)(ptr-(char *)req + 4));

          /* setup data */
          // _req = (stun_hdr_t *)tdata;
          tmplen = ptr - (char *) req;
          x_memset(tdata, 0, sizeof(tdata));
          x_memcpy(tdata, (char *) req, tmplen);
          hdr = (stun_hdr_t *) tdata;
          hdr->mlen = htons(tmplen - sizeof(stun_hdr_t) + 24);

          // Test print
          // hexdump(ptr+4,20);
          TRACE("MLEN(%d)(%d)(%d)\n",
              (int)sizeof(stun_hdr_t), (int)tmplen, (int)(tmplen - sizeof(stun_hdr_t) + 24));

          // Calc Message integrity
          msg_integrity = jingle_get_hmac_sha1("1234", 4, (char *) tdata,
              tmplen);
          if (msg_integrity)
            x_free(msg_integrity);
        }

      ptr += sizeof(uint16_t);
      pkt->attrs[pkt->attrnum].len = ntohs(*((uint16_t *) ptr));
      ptr += sizeof(uint16_t);

      // if attribute has value
      if (pkt->attrs[pkt->attrnum].len)
        {
          // copy data
          pkt->attrs[pkt->attrnum].data = x_malloc(
              pkt->attrs[pkt->attrnum].len);
          x_memcpy(pkt->attrs[pkt->attrnum].data, ptr,
              pkt->attrs[pkt->attrnum].len);
          ptr += ADDRLEN_ALIGN(pkt->attrs[pkt->attrnum].len);
        }
      else
        pkt->attrs[pkt->attrnum].data = NULL;

      TRACE("-- ATTRIBUTE, (%d): TYPE[0x%x],LEN[%d],DATA[%s]\n",
          pkt->attrnum, pkt->attrs[pkt->attrnum].type, pkt->attrs[pkt->attrnum].len, pkt->attrs[pkt->attrnum].data);

      pkt->attrs[++pkt->attrnum].type = 0;
    }

  crc = stun_get_fingerprint(req);
  TRACE("-- CRC (0x%x[0x%x])\n", crc, crc^STUN_FINGERPRINT_XOR);

  EXIT;

  return pkt;
}

int
stun_attr_id(stun_packet_t *pkt, int type)
{
  stun_attr_t *attrs;
  int i;

  for (attrs = pkt->attrs, i = 0; i < (int) pkt->attrnum; i++)
    {
      if (attrs[i].type == (uint16_t) type)
        break;
    }
  // TRACE("@@@ ATTRNUM %d\n",i);
  i = ((i >= 0) && (i < pkt->attrnum)) ? i : -1;
  //  TRACE("@@@ ATTRNUM %d\n",i);
  return i;
}

int
stun_from_x_mapped(char *src, struct sockaddr_in *dst)
{
  uint16_t port;
  uint32_t ip;

  ENTER;
  port = *(uint16_t *) (src + 2);
  ip = *(uint32_t *) (src + 4);

  ip = ntohl(ip);
  port = ntohs(port);
  port ^= 0x2112;
  ip ^= STUN_M_COOKIE;

  dst->sin_port = htons(port);
  dst->sin_addr.s_addr = htonl(ip);

  EXIT;
  return 0;
}

