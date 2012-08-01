#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <malloc.h>
#include <process.h>
#else
#include <alloca.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>
#endif

typedef void
(udp_callback_fn_t)(void *data, int len, void *cb_data);

int
udp_connect(const char *host, int port)
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

  if (connect(sock, (struct sockaddr *) &saddr, sizeof(saddr)))
    return -1;

  return sock;
}

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
    return -1;

  return sock;
}

static char buf[8192*10];

int
udp_listen(const char *host, int port, udp_callback_fn_t *cb_func,
    void *cb_data)
{
  char buf2[1024];
  int p_total = 0;
  int lastsiz = 0;
  int sock;
  struct sockaddr_in saddr;
  struct sockaddr_in cli;
  size_t addrsiz = sizeof(cli);

  sock = udp_bind(host, port);
  if (sock < 0)
    return -1;
  memset(&cli, 0, sizeof(cli));
  memset(buf, 0, sizeof(buf));

  // printf("[udp] 1\n");

  for (;;)
    {
    // printf("[udp]\n");
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

