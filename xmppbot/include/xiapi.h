#ifndef __XIAPI_H__
#define __XIAPI_H__

#include <xi.h>
#include <ev.h>

struct xiapi_server;

#define XIAPI_STATE_UNREADY     0
#define XIAPI_STATE_STOPPED     1
#define XIAPI_STATE_RUNNING     2

struct xiapi_server
{
  int sockfd;
  char *server_name;
  int state;
  struct ev_loop *loop;
  struct ev_io srv_watcher;
  int (*on_create) (struct xiapi_server *xis);
  int (*on_start) (struct xiapi_server *xis, struct ev_loop *el);
  int (*on_stop) (struct xiapi_server *xis);
  int (*on_destroy) (struct xiapi_server *xis);
  int maxbuflen;
  char *data;
};

#endif /* __XIAPI_H__ */
