/*
 * x_stun.h
 *
 *  Created on: 17.11.2010
 *      Author: artur
 */

#ifndef X_STUN_H_
#define X_STUN_H_

#ifdef __cplusplus
extern "C"
  {
#endif

#include <sys/types.h>
#include <xwlib/x_types.h>

#define STUN_PORT_DEFAULT 0x0D96
#define STUN_PORT_DEFAULT_N 0x960D

#define STUN_M_COOKIE 0x2112A442
#define STUN_NET_M_COOKIE       htonl(STUN_M_COOKIE)

#define STUN_TRANS_ID_LEN 12
#define STUN_REQ        0x0001
#define STUN_RESP       0x0101
#define STUN_USERNAME   0x0006
#define STUN_PASSWORD   0x0007

#define STUN_CHANGE_REQUEST     0x0003
#define STUN_MSG_INTEGRITY      0x0008
#define STUN_MAPPED_ADDRESS 0x0001
#define STUN_X_MAPPED_ADDRESS 0x0020
#define STUN_PRIORITY   0x0024
#define STUN_FINGERPRINT        0x8028
#define STUN_ICE_CONTROLLED     0x8029
#define STUN_ICE_CONTROLLING    0x802a

#define STUN_FINGERPRINT_XOR 0x5354554e

#define ADDRLEN_ALIGN(x) ((x + (sizeof(uint32_t) - 1))&~(sizeof(uint32_t) - 1))

/**
 *************** STUN *******************
 */
typedef struct stun_hdr
{
  union
  {
#ifdef  __LE__
    uint16_t mtype:14; // padding
    uint16_t v:2;// version
#else
    uint16_t v :2; // extensions
    uint16_t mtype :14; // count of contributing sources
#endif
    uint16_t raw;
  } ver_type;
  uint16_t mlen; // message length
  uint32_t mcookie; // magic-cookie
  char trans_id[STUN_TRANS_ID_LEN]; // transaction id
  char payload[]; // message data
} stun_hdr_t;

typedef struct stun_attr
{
  uint16_t type;
  uint16_t len;
  char *data;
} stun_attr_t;

typedef struct stun_packet
{
  int len;
  stun_hdr_t *hdr;
  unsigned int attrnum;
  stun_attr_t *attrs;
} stun_packet_t;

EXPORT_SYMBOL stun_packet_t *
stun_packet_from_hdr(stun_hdr_t *req, const char *key);
EXPORT_SYMBOL int
stun_attr_id(stun_packet_t *pkt, int type);
EXPORT_SYMBOL int
stun_from_x_mapped(char *src, struct sockaddr_in *dst);
EXPORT_SYMBOL stun_hdr_t *
get_stun_simple_request(void);
EXPORT_SYMBOL stun_hdr_t *
get_stun_full_request(const char *local_ufrag, const char *remote_ufrag,
    const char *remote_pwd);
EXPORT_SYMBOL stun_hdr_t *
stun_get_response(const char *stun_key, stun_hdr_t *req, struct sockaddr *saddr);
EXPORT_SYMBOL void
stun_pkt_free(stun_packet_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* X_STUN_H_ */
