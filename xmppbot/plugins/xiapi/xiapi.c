/*
 * Copyright (c) 2011, artemka
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
 * xiapi.c
 *
 * Backend side of external interaction API.
 * It provides way for safe access to or control of internal
 * data structures (contact list, sessions, messaging system etc.).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[xiapi] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>

#include <xiapi.h>
#include <xmppagent.h>

static struct xiapi_server xis;

static int
__xiapi_test(char *data)
{
  const char *reply = "OK";
  xi_hdr_t *hdr = (xi_hdr_t *) data;

  ENTER;

  TRACE("message from xilib: %s\n", data + sizeof(*hdr));
  hdr->data_size = strlen(reply) + 1;
  strncpy(data + sizeof(*hdr), reply, hdr->data_size);

  EXIT;
  return 0;
}

static void
__xiapi_server_cb(struct ev_loop *loop, struct ev_io *data, int mask)
{
  struct sockaddr_un client_address;
  int fromlen;
  struct xiapi_server *xis = (struct xiapi_server *) (((char *) data)
      - ((char *) &((struct xiapi_server *) 0)->srv_watcher));
  xi_hdr_t *hdr;

  ENTER;

  if (!xis)
    {
      TRACE("error: wrong watcher\n");
      EXIT;
      return;
    }

  fromlen = sizeof(client_address);
  memset(&client_address, 0, fromlen);
  if (recvfrom(xis->sockfd, xis->data, xis->maxbuflen, 0,
      (struct sockaddr *) &client_address, (socklen_t *) &fromlen) < 0)
    {
      TRACE("recvfrom failed (%d:%s)\n", errno, strerror(errno));
      EXIT;
      return;
    }

  hdr = (xi_hdr_t *) xis->data;

  switch (hdr->op)
    {
  case XI_NONE:
    break;

  case XI_TEST:
    hdr->err = __xiapi_test(xis->data);
    break;

    /* TODO:  */
  case XI_GET_BUS_CTRL:
  case XI_SET_BUS_CTRL:
  case XI_GET_CONTACTS:
  case XI_GET_CONTACT_INFO:
  case XI_SET_CONTACT_INFO:
  case XI_GET_SESSIONS:
  case XI_GET_SESSION_INFO:
  case XI_SET_SESSION_INFO:
  case XI_ON_CONTACT_PRESENCE:
  case XI_MSG_SESSION_CREATE:
  case XI_STD_SESSION_CREATE:
  case XI_ON_MSG_SESSION_NEW:
  case XI_ON_STD_SESSION_NEW:
    TRACE("<NI>\n");
    break;

  default:
    break;
    }

  if (sendto(xis->sockfd, xis->data, sizeof(*hdr) + hdr->data_size, 0,
      (const struct sockaddr *) &client_address, sizeof(client_address)) < 0)
    {
      TRACE("sendto failed (%d:%s)\n", errno, strerror(errno));
      EXIT;
      return;
    }

  EXIT;
  return;
}

static int
___xiapi_create_server(struct xiapi_server *xis)
{
  int sockfd;
  struct sockaddr_un server_address;
  char *buf;
  int maxbuflen;

  ENTER;

  sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      TRACE("server: opening socket failed\n");
      EXIT;
      return -1;
    }

  memset(&server_address, 0, sizeof(server_address));

  server_address.sun_family = AF_UNIX;
  strncpy(server_address.sun_path, xis->server_name,
      sizeof(server_address.sun_path) - 1);
  unlink(xis->server_name);

  if (bind(sockfd, (struct sockaddr *) &server_address, sizeof(server_address))
      == -1)
    {
      close(sockfd);
      TRACE("server: bind failed\n");
      EXIT;
      return -1;
    }

  /* switch it to async nonblocking operation mode */
  fcntl(sockfd, F_SETFL, O_NONBLOCK | O_ASYNC);
  // fcntl(sockfd, F_SETFL, O_ASYNC);

  maxbuflen = getpagesize();
  buf = (char *) malloc(maxbuflen);
  if (buf == NULL)
    {
      close(sockfd);
      TRACE("server: failed to allocate memory\n");
      EXIT;
      return -1;
    }

  xis->data = buf;
  xis->sockfd = sockfd;
  xis->state = XIAPI_STATE_STOPPED;
  xis->maxbuflen = maxbuflen;
  ev_io_init(&xis->srv_watcher, __xiapi_server_cb, xis->sockfd, EV_READ /*| EV_WRITE*/);

  EXIT;
  return sockfd;
}

static int
___xiapi_destroy_server(struct xiapi_server *xis)
{
  if (xis->state != XIAPI_STATE_STOPPED)
    {
      TRACE("couldn't destroy xiapi server - not ready for that\n");
      EXIT;
      return -1;
    }

  xis->state = XIAPI_STATE_UNREADY;
  xis->loop = NULL;
  close(xis->sockfd);
  xis->sockfd = -1;
  xis->maxbuflen = 0;
  free(xis->data);
  xis->data = NULL;

  return 0;
}

static int
__xiapi_start_server(struct xiapi_server *xis, struct ev_loop *el)
{
  ENTER;
  if (xis->state != XIAPI_STATE_STOPPED)
    {
      TRACE("couldn't start xiapi server - not ready\n");
      EXIT;
      return -1;
    }

  xis->state = XIAPI_STATE_RUNNING;
  xis->loop = el;
  ev_io_start(xis->loop, &xis->srv_watcher);

  EXIT;
  return 0;
}

static int
__xiapi_stop_server(struct xiapi_server *xis)
{
  if (xis->state != XIAPI_STATE_RUNNING)
    {
      TRACE("couldn't stop xiapi server - not running\n");
      EXIT;
      return -1;
    }

  xis->state = XIAPI_STATE_STOPPED;
  ev_io_stop(xis->loop, &xis->srv_watcher);

  EXIT;
  return 0;
}

static struct xiapi_server xis =
  { .sockfd = -1, .state = XIAPI_STATE_UNREADY, .server_name = XI_SRV_PATH,
      .on_create = &___xiapi_create_server,
      .on_destroy = &___xiapi_destroy_server,
      .on_start = &__xiapi_start_server, .on_stop = &__xiapi_stop_server,
      .maxbuflen = 0, .data = NULL, };

static void
___xiapi_on_x_path_event(void *_obj)
{
  x_object *tmp;
  x_object *obj = _obj;

  ENTER;

  xis.on_start(&xis, ((struct x_bus *) obj->bus)->obj.loop);

  tmp = x_object_add_path(obj, "__proc/xiapi", NULL);
  x_object_setattr(tmp, "name", "Xiapi Server v1.0");

  EXIT;
}

static struct x_path_listener xiapi_bus_listener =
  { .on_x_path_event = &___xiapi_on_x_path_event, };

static __plugin_init void
xiapi_init(void)
{
  ENTER;

  if (xis.on_create(&xis) < 0)
    {
      TRACE("failed to init xiapi plugin\n");
      EXIT;
      exit(-1);
    }

  x_path_listener_add("/", &xiapi_bus_listener);

  EXIT;
}

PLUGIN_INIT(xiapi_init);

