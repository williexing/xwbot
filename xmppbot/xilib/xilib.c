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
 * xilib.c
 *
 * Frontend side of external interaction API.
 * It is a standalone library which is an entry point for external control software,
 * like GUI or automation software. It provides communication with Gobee internals
 * through the backend via a defined protocol which is one of
 * request/response derivatives
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <xwlib/x_utils.h>
#include "xilib.h"

#undef DEBUG_PRFX
#define DEBUG_PRFX "[xilib] "

static int
__xilib_process_message(xilib_comm_t *comm);

int
xilib_open(xilib_comm_t *comm)
{
  int sockfd;
  struct sockaddr_un server_address;
  struct sockaddr_un client_address;

  ENTER;

  sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      TRACE("opening socket failed\n");
      EXIT;
      return -1;
    }

  memset(&client_address, 0, sizeof client_address);
  client_address.sun_family = AF_UNIX;
  if (bind(sockfd, (const struct sockaddr *) &client_address,
      sizeof(struct sockaddr_un)) < 0)
    {
      TRACE("bind failed\n");
      EXIT;
      return -1;
    }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sun_family = AF_UNIX;
  strncpy(server_address.sun_path, XI_SRV_PATH, sizeof(server_address.sun_path)
      - 1);

  memset(comm, 0, sizeof(*comm));
  comm->sockfd = sockfd;
  comm->server_address = server_address;

  EXIT;
  return 0;
}

__inline__ void
xilib_close(xilib_comm_t *comm)
{
  close(comm->sockfd);
  memset(comm, 0, sizeof(*comm));
}

int
xilib_test(xilib_comm_t *comm)
{
  const char *testmsg = "My son, ask for thyself another kingdom, "
    "For that which I leave is too small for thee";
  xi_hdr_t hdr;
  char in_data[20];
  int ret;

  ENTER;

  hdr.op = XI_TEST;
  hdr.data_size = strlen(testmsg) + 1;
  hdr.err = 0;

  comm->out_size = sizeof(hdr) + hdr.data_size;
  comm->data_out = (char *) malloc(comm->out_size);
  if (!comm->data_out)
    {
      TRACE("memory allocation failed\n");
      EXIT;
      return -1;
    }
  strncpy(comm->data_out + sizeof(hdr), testmsg, strlen(testmsg) + 1);

  *((xi_hdr_t *) comm->data_out) = hdr;

  memset(in_data, 0, sizeof(in_data));
  comm->data_in = in_data;
  comm->in_size = sizeof(in_data);

  ret = __xilib_process_message(comm);
  if (ret < 0)
    {
      free(comm->data_out);
      TRACE("communication error\n");
      EXIT;
      return -1;
    }

  hdr = *((xi_hdr_t *) comm->data_in);
  if (hdr.op != XI_TEST || hdr.err != 0)
    {
      free(comm->data_out);
      TRACE("some error on server side\n");
      EXIT;
      return -1;
    }

  printf("xiapi response: %s\n", (char *) (comm->data_in + sizeof(hdr)));

  free(comm->data_out);

  EXIT;
  return 0;
}

static int
__xilib_process_message(xilib_comm_t *comm)
{
  socklen_t fromlen;

  ENTER;

  printf("sending ... ");
  if (sendto(comm->sockfd, comm->data_out, comm->out_size, 0,
      (const struct sockaddr *) &comm->server_address,
      sizeof(comm->server_address)) < 0)
    {
      printf("sendto failed\n");
      EXIT;
      return -1;
    }
  printf("done\n");

  printf("receiving ... ");
  fromlen = sizeof(comm->server_address);
  if (recvfrom(comm->sockfd, comm->data_in, comm->in_size, 0,
      (struct sockaddr *) &comm->server_address, &fromlen) < 0)
    {
      printf("recvfrom failed\n");
      EXIT;
      return -1;
    }
  printf("done\n");

  EXIT;
  return 0;
}
