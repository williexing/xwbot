/*
 * xmppagent.h
 *
 *  Created on: Aug 4, 2010
 *      Author: artur
 */

#ifndef XMPPAGENT_H_
#define XMPPAGENT_H_

#define XMPP_PORT 5222
#define BUFLEN 1024

#ifdef WITH_SSL
#	include <openssl/ssl.h>
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>
#include <x_parser.h>

#include <ev.h>

#define LOG_XMPPP 1

#define XMPP_PORT_STR "5222"
#define XMPP_PORT 5222

#define GTALK_HOSTNAME "talk.google.com"
#define VK_HOSTNAME "vkmessenger.ru"
#define QIP_HOSTNAME "xmpp-server.qip.ru"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Structure representing xmpp session
 * @ingroup XBUS
 */
struct x_bus
{
  /** superclass for x_bus */
  x_object obj;

  struct {
    int state;
    /* current object in tree */
    x_object *r_obj;
    /*  */
  } regs;

  /** xml push parser */
  struct x_push_parser *p;

  /** xmpp connection socket */
  int sys_socket;

  /** transport abstraction */
  struct x_transport *transport;

  struct ev_loop *loop;
  struct ev_io io_watcher;

  /** object factory instance */
  /*struct xobject_factory *o_factory;*/

  /** shared event/state/message wall used
   * by plugins to share common data */
//  struct ht_cell *wall[HTABLESIZE];

  /**
   * Namespace map
   */
  struct ht_cell *nsmap[HTABLESIZE];

#ifdef LOG_XMPPP
  int xmppoutfd;
  int xmppinfd;
#endif


};

enum
{
  BUS_STATE_INI = 0,
  BUS_STATE_RESET,
  BUS_STATE_CONNECTED,
  BUS_STATE_STREAMING,
  BUS_STATE_CLOSED,
};

/**
 * @defgroup XBUS Xbus API
 *
 * @{
 */
EXPORT_SYMBOL int x_bus_write(struct x_bus *, void *, size_t, int);
EXPORT_SYMBOL void x_bus_reset_stream(struct x_bus *bus);
EXPORT_SYMBOL void x_bus_set_transport(struct x_bus *bus, struct x_transport *t);
/**
 * @}
 */
#ifdef WITH_SSL
void xmpp_stop_tls(struct x_bus *sess);
int xmpp_start_tls (struct x_bus *sess);
#endif


#ifdef __cplusplus
};
#endif

#endif /* XMPPAGENT_H_ */
