#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "mynet.h"

typedef void
( udp_callback_fn_t)(void *data, int len, void *cb_data);

int
udp_bind(const char *host, int port)
{
  int sock;
  struct sockaddr_in saddr;

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0)
    return -1;

  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = inet_addr(host);
  saddr.sin_port = htons(port);

  if (bind(sock, (struct sockaddr *) &saddr, sizeof(saddr)))
    {
      printf("bind() error\n");
      return -1;
    }

  printf("bind() ok\n");
  return sock;
}

int
udp_listen(const char *host, int port, udp_callback_fn_t *cb_func,
    void *cb_data)
{
  char buf[8192];
  int lastsiz = 0;
  int sock;
  struct sockaddr_in cli;
  socklen_t addrsiz = (socklen_t)sizeof(cli);

  sock = udp_bind(host, port);
  if (sock < 0)
    return -1;
  memset(&cli, 0, sizeof(cli));
  memset(buf, 0, sizeof(buf));

  for (;;)
    {
      lastsiz = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &cli,
          &addrsiz);
      if (lastsiz > 0)
        {
          if (cb_func)
            cb_func(buf, lastsiz, cb_data);
        }
    }
  return 0;
}


/**
 * @todo Use vectorized sendmsg() instead of sendto()
 * for optimization
 */
/*
int
rtp_send(int fd, const void *buf, int count, struct sockaddr *saddr,
    int addrlen, unsigned int t_stamp, int ptype, int seqno)
{
  rtp_hdr_t *_rtp;
  int len = sizeof(rtp_hdr_t) + count;

  _rtp = (rtp_hdr_t *) memset(malloc(len), 0, len);

  _rtp->octet1.control1.v = 2;
  _rtp->octet1.control1.p = 0;
  _rtp->octet1.control1.x = 0;
  _rtp->octet1.control1.cc = 0;
  _rtp->octet2.control2.m = 0;

  _rtp->ssrc_id = htonl(0x44332200);
  _rtp->seq = htons(seqno);
  _rtp->octet2.control2.pt = ptype & 0x7f;

  _rtp->t_stamp = htonl(t_stamp);
  memcpy(_rtp->payload, buf, count);

  printf("Sending %d bytes\n", len);

  len = sendto(fd, _rtp, len, 0, saddr, addrlen);

  free(_rtp);

  return len;
}

*/


int
rtp_send(int fd, const void *buf, int count, struct sockaddr *saddr,
    int addrlen, unsigned int t_stamp, int ptype, int _seqno)
{
  rtp_hdr_t *_rtp;
  int len = sizeof(rtp_hdr_t) + count;

  _rtp = (rtp_hdr_t *) memset(malloc(len), 0, len);
  _rtp->octet1.control1.v = 2;
  _rtp->octet1.control1.p = 0;
  _rtp->octet1.control1.x = 0;
  _rtp->octet1.control1.cc = 0;
  _rtp->octet2.control2.m = 0;

  _rtp->ssrc_id = htonl(0x44332200);
  _rtp->seq = htons(_seqno++);
  _rtp->octet2.control2.pt = ptype & 0x7f;

  _rtp->t_stamp = htonl(t_stamp);
  memcpy(_rtp->payload, buf, count);

  // printf("[rtp] seqno=%d, sending %d bytes...", seqno, len);
  len = sendto(fd, _rtp, len, 0, saddr, addrlen);

  free(_rtp);

  return len;
}

