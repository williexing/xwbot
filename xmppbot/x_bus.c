/*
 * xmppagent.c
 *
 *  Created on: Aug 4, 2010
 *      Author: artur
 */

#define _BSD_SOURCE
#define _POSIX_C_SOURCE 1

#undef DEBUG_PRFX

#include <xwlib/x_config.h>
#if TRACE_XBUS_ON
#define DEBUG_PRFX "[bus] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_lib.h>
#include <xwlib/x_obj.h>

#include <xmppagent.h>
#include <plugins/sessions_api/sessions.h>

#include <ev.h>

static void
ev_show_backends(void)
{
  int flags = ev_recommended_backends();
  char *backends = (char *) x_malloc(256);
  char *tmp = backends;

  tmp += sprintf(tmp, "%s ", "System supports: ");

  if (flags & EVBACKEND_SELECT)
    tmp += sprintf(tmp, "%s ", "select");

  if (flags & EVBACKEND_POLL)
    tmp += sprintf(tmp, "%s ", "poll");

  if (flags & EVBACKEND_EPOLL)
    tmp += sprintf(tmp, "%s ", "epoll");

  if (flags & EVBACKEND_KQUEUE)
    tmp += sprintf(tmp, "%s ", "kqueue");

  if (flags & EVBACKEND_DEVPOLL)
    tmp += sprintf(tmp, "%s ", "devpoll");

  if (flags & EVBACKEND_PORT)
    sprintf(tmp, "%s ", "port");

  TRACE("%s\n", backends);
  x_free(backends);
}

extern struct x_transport *
xmpp_start_tls(int sock);

static int
__x_bus_objopen(const char *name, x_obj_attr_t *attrs, void *dat);
static int
__x_bus_objclose(const char *curobj, void *dat);
static __inline__ int
__x_bus_putchar(char c, void *dat);

__inline__ static void
__x_bus_set_state(struct x_bus *bus, int state)
{
  ENTER;
  bus->regs.state = BUS_STATE_CONNECTED;
  EXIT;
}

/* transmit object downstream */
static int
x_bus_tx(x_object *o, x_object *msg, x_obj_attr_t *hints)
{
  x_string xstr =
    { 0, 0, 0 };

  struct x_bus *bus = (struct x_bus *) o;

  ENTER;

  x_string_clear(&xstr);
  /* @todo
   * FIXME this should be optimized to use transport callbacks
   * instead of x_string transcoding
   */
  x_parser_object_to_string(msg, &xstr);
  x_bus_write(bus, xstr.cbuf, xstr.pos, 0);

  _REFPUT(msg, NULL);

  if (hints)
    {
      attr_list_clear(hints);
      free(hints);
    }

  EXIT;

  return 0;
}

/**
 *
 */
struct dfl_socket_transport
{
  struct x_transport super;
  int sock;
};

static int
sock_write(struct x_transport *ss, char *buf, size_t len, int flags)
{
  int err;
  struct dfl_socket_transport *st = (struct dfl_socket_transport *) ss;
  ENTER;
  err = send(st->sock, buf, len, flags);
  EXIT;
  return err;
}

static int
sock_read(struct x_transport *ss, char *buf, size_t len, int flags)
{
  int err;
  struct dfl_socket_transport *st = (struct dfl_socket_transport *) ss;
  err = recv(st->sock, buf, len, flags);
  if (err < 0)
    {
      if (errno == EWOULDBLOCK)

        {
          err = 0;
          TRACE("Blocking...\n");
        }
    }
  else
    {
      ;
    }
  return err;
}

static struct x_transport *
x_bus_dfl_transport_create(struct x_bus *bus)
{
  struct dfl_socket_transport *st = (struct dfl_socket_transport *) x_malloc(
      sizeof(struct dfl_socket_transport));
  x_memset((void*) st, 0, sizeof(struct dfl_socket_transport));
  st->sock = bus->sys_socket;
  st->super.rd = &sock_read;
  st->super.wr = &sock_write;
  return (struct x_transport *) st;
}

/**
 * Creates new session
 */
static void
__x_bus_on_create(x_object *o)
{
  struct x_bus *bus = (struct x_bus *) (void *) o;

  ENTER;
  BUG_ON(!bus);

  /* initialize xbus registers */
  bus->regs.r_obj = X_OBJECT(bus);
  __x_bus_set_state(bus, BUS_STATE_INI);

  /* create event loop
   * @todo This should be done in the finalize handler
   */
  bus->obj.loop = ev_default_loop(EVFLAG_AUTO);

  /* allocate xmpp parser */
  //x_parser_init(&bus->p);
  bus->p = (struct x_push_parser *) x_object_new_ns("#xmppparser", "gobee");
  BUG_ON(!bus->p);

  /* set parser callbacks */
  bus->p->xobjopen = &__x_bus_objopen;
  bus->p->xobjclose = &__x_bus_objclose;
  bus->p->xputchar = &__x_bus_putchar;
  bus->p->cbdata = (void *) bus;

  /* set object root */
  bus->obj.bus = (x_object *) bus;

//  x_memset(&bus->wall[0], 0, sizeof(bus->wall));

#ifndef WIN32
#ifdef LOG_XMPPP
  bus->xmppoutfd = open("xmpp_out.log", O_CREAT | O_TRUNC | O_SYNC | O_RDWR,
      S_IRWXU | S_IRWXG);
  bus->xmppinfd = open("xmpp_in.log", O_CREAT | O_TRUNC | O_SYNC | O_RDWR,
      S_IRWXU | S_IRWXG);
#endif
#endif

  EXIT;
  return;
}

/**
 * Fills session attributes from jid
 */
static int
__x_bus_jid_commit(x_object *bus)
{
  int len;
  const char *jid;
  char *ptr = NULL;
  char *ptr1 = NULL;
  char *ptr2 = NULL;

  ENTER;

  BUG_ON(!bus);

  jid = x_object_getattr(bus, "jid");
  if (!jid)
    {
      TRACE();
      exit(-1);
    }

  len = strlen(jid);
  ptr = x_strdup(jid);

  ptr1 = strchr(ptr, '@');
  if (!ptr1)
    return -1;
  *ptr1++ = '\0';

  ptr2 = strchr(ptr1, '/');
  if (!ptr2)
    return -1;

  *ptr2++ = '\0';

  /* alloc and copy username */
  _ASET((struct xobject *) bus, "username", ptr);

  /* alloc and copy home room */
  _ASET((struct xobject *) bus, "resource", ptr2 ? ptr2 : "gobee");

  /* alloc and copy uname and port */
  ptr2 = strchr(ptr1, ':');
  if (ptr2)
    {
      *ptr2++ = '\0';
      _ASET((struct xobject *) bus, "servicename", ptr2);
    }
  else
    {
      _ASET((struct xobject *) bus, "servicename", XMPP_PORT_STR);
    }

  /* alloc and copy hostname */
  _ASET((struct xobject *) bus, "domain", ptr1);

  /* if hostname is not set, then get it from jid */
  if (!_AGET((struct xobject *) bus, "hostname"))
    {
      if (EQ(ptr1, "gmail.com"))
        {
          _ASET((struct xobject *) bus, "hostname", GTALK_HOSTNAME);
        }
      else if (EQ(ptr1, "vk.com"))
        {
          _ASET((struct xobject *) bus, "hostname", VK_HOSTNAME);
        }
      else
        {
          _ASET((struct xobject *) bus, "hostname", ptr1);
        }
    }

  /* clear memory */
  if (ptr)
    x_free(ptr);

#ifdef ATTRLIST_DEBUG
  print_attr_list(&bus->attrs);
#endif

  EXIT;
  /* no error */
  return 0;
}

/**
 * Connects to server
 */
static int
__x_bus_connect(struct x_bus *bus)
{
  struct addrinfo hints;
  const char *hostname;
  const char *servicename;
  int err = -1;
  struct addrinfo *res = NULL;

  ENTER;
  if (!bus)
    {
      TRACE("Invalid session\n");
      EXIT;
      return -1;
    }

  memset(&hints, 0, sizeof(hints));
  /* set-up hints structure */
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  hostname = x_object_getattr((struct xobject *) bus, "hostname");
  servicename = x_object_getattr((struct xobject *) bus, "servicename");

  TRACE("Connecting to %s\n", hostname);

  err = getaddrinfo(hostname, servicename ? servicename : "5222", &hints, &res);
  if (err)
    {
      TRACE("ERROR!! getaddrinfo(): %s", gai_strerror(err));
      goto clean_and_exit;
    }

  TRACE("%p (%s:%d)\n", (void *)res->ai_addr, __FILE__, __LINE__);

  switch (res->ai_addr->sa_family)
    {
  case AF_INET:
    bus->sys_socket = socket(AF_INET, SOCK_STREAM, 0);

    TRACE("sock %d: IPv4 Address %d.%d.%d.%d:%d.%d\n",
        bus->sys_socket, res->ai_addr->sa_data[0], res->ai_addr->sa_data[1], res->ai_addr->sa_data[2], res->ai_addr->sa_data[3], res->ai_addr->sa_data[4], res->ai_addr->sa_data[5]);
    break;

  case AF_INET6:
    bus->sys_socket = socket(AF_INET6, SOCK_STREAM, 0);
    TRACE("IPv6 Address %02x:%02x:%02x:%02x:%02x:%02x\n",
        res->ai_addr->sa_data[0], res->ai_addr->sa_data[1], res->ai_addr->sa_data[2], res->ai_addr->sa_data[3], res->ai_addr->sa_data[4], res->ai_addr->sa_data[5]);
    break;

  default:
    TRACE("UNKNOWN Address family\n");
    goto clean_and_exit;
    /*break;*/
    }

  err = connect(bus->sys_socket, res->ai_addr, res->ai_addrlen);
  if (err)
    {
      TRACE("ERROR!! connect(): sock %d, addrlen=%d: %s\n",
          bus->sys_socket, res->ai_addrlen, strerror(errno));
      //close(bus->sys_socket);
      bus->sys_socket = -1;
      goto clean_and_exit;
    }

  /* set default transport */
  bus->transport = x_bus_dfl_transport_create(bus);

  /*  */
  __x_bus_set_state(bus, BUS_STATE_CONNECTED);

  err = 0;
  TRACE("Ok! Connected! Socket(%d)\n", bus->sys_socket);

  /* switch it to async nonblocking operation mode */
  set_sock_to_non_blocking(bus->sys_socket);

  clean_and_exit: if (res)
    freeaddrinfo(res);
  return err;
}

/**
 * This handles socket events
 */
static void
__x_bus_on_signal_data(struct ev_loop *loop, struct ev_io *data, int mask)
{
  int err, i;
  char buf[256];
  struct x_bus *bus;

  buf[255] = 0;

  bus = (struct x_bus *) (((char *) data) - offsetof(struct x_bus, io_watcher));

  if (!bus)
    return;

  if (mask & EV_READ)
    {
#if 0
      if ((err = bus->transport->rd(bus->transport, buf, sizeof(buf) - 1, 0))
          > 0)
#else
      while ((err = bus->transport->rd(bus->transport, buf, sizeof(buf) - 1, 0))
          > 0)
#endif
        {
          ((char *) buf)[err] = 0;
          TRACE("\n%s\n", buf);

          for (i = 0; i < err; i++)
            {
              bus->p->r_putc(bus->p, buf[i]);
            }
        }
    }
  else if (mask & EV_WRITE)
    {
      if (bus->transport->on_wr)
        bus->transport->on_wr(bus->transport, buf, sizeof(buf) - 1, 0);
    }

  return;
}

static void
__x_bus_fill_nsmap(struct x_bus *bus, x_obj_attr_t *attrs)
{
  int ind;
  char *ptr;
  x_obj_attr_t *p;

  for (p = attrs->next; p; p = p->next)
    {
      if (!x_strncasecmp(p->key, "xmlns:", sizeof("xmlns:") - 1))
        {
          ptr = x_strchr(p->key, ':') + 1;

          //          if (!ht_get(ptr, &bus->nsmap), &ind)
          ht_set(ptr, x_strdup(p->val), &bus->nsmap);
        }
    }
}

/**
 * Allocates object
 */
static int
__x_bus_objopen(const char *_name, x_obj_attr_t *attrs, void *dat)
{
  x_object *o_;
  struct x_bus *bus = (struct x_bus *) dat;
  const char *name, *__name = _name;
  const char *_ns = NULL;
  const char *ptr = NULL;
  x_string ns =
    { 0, 0, NULL };
  x_string ns2 =
    { 0, 0, NULL };
  x_string sname =
    { 0, 0, NULL };
  char tmpbuf[64];
  int has_ns = 0;
  int ind;

  ENTER;

  /* copy element's namespace */
  while (*_name && *_name != 0)
    {
      if (*_name == ':')
        {
          has_ns = 1;
          _name++;
          break;
        }
      else
        x_string_putc(&ns, *_name++);
    }

  /* search for 'xmlns:*' attributes */
  __x_bus_fill_nsmap(bus, attrs);

  /**
   * fixme Ugly hack
   * @todo Ugly hack
   */
  if (has_ns)
    {
      while (*_name && *_name != 0)
        {
          x_string_putc(&sname, *_name++);
        }
      name = sname.cbuf;
      _ns = ht_get(ns.cbuf, &bus->nsmap, &ind);
    }
  else
    {
      name = ns.cbuf;
      if (!(_ns = getattr("xmlns", attrs)))
        _ns = _ENV(bus->regs.r_obj,"xmlns");
    }

  if ((ptr = x_strchr(_ns, '#')))
    {
      x_string_write(&ns2, _ns, ptr - _ns);
      _ns = ns2.cbuf;
    }

  TRACE("Opening '%s' object from '%s' namespace\n", name, _ns);

  if ((o_ = _FIND(x_object_get_child(bus->regs.r_obj, name), attrs)) != NULL)
    goto objopen_end;

  /* instantiate object from class */
  if (!(o_ = x_object_new_ns_match(name, _ns, attrs, TRUE)))
    {
      TRACE("Error creating '%s' object\n", name);
      return -1;
    }

  /* Append child object */
  if (_INS(bus->regs.r_obj, o_))
    {
      TRACE("ERROR!");
      return -1;
    }

  objopen_end: bus->regs.r_obj = o_;

  x_object_assign(o_, attrs);

  EXIT;

  x_string_trunc(&ns);
  x_string_trunc(&ns2);
  x_string_trunc(&sname);
  return 0;
}

/**
 * Closes object returns its parent
 */
static int
__x_bus_objclose(const char *_name, void *dat)
{
  struct x_bus *bus = (struct x_bus *) dat;
  x_object *o_ = bus->regs.r_obj;
  const char *name, *__name = _name;
  const char *_ns = NULL;
  x_string ns =
    { 0, 0, NULL };
  x_string sname =
    { 0, 0, NULL };
  int has_ns = 0;
  int ind;

  // copy namespace
  // copy namespace
  while (*_name && *_name != 0)
    {
      if (*_name == ':')
        {
          has_ns = 1;
          _name++;
          break;
        }
      else
        x_string_putc(&ns, *_name++);
    }

  if (has_ns)
    {
      while (*_name && *_name != 0)
        {
          x_string_putc(&sname, *_name++);
        }
      name = sname.cbuf;
      _ns = ht_get(ns.cbuf, &bus->nsmap, &ind);
    }
  else
    {
      name = ns.cbuf;
      _ns = _ENV(o_,"xmlns");
    }

  if (x_strcasecmp(name, _GETNM(o_)))
    {
      /* TODO */
#pragma message ( "TODO Finish this!!!")
      return -1;
    }

  bus->regs.r_obj = x_object_get_parent(o_);

  x_object_lost_focus(o_);

  x_string_trunc(&ns);
  x_string_trunc(&sname);

  return 0;
}

static __inline__ int
__x_bus_putchar(char c, void *dat)
{
  struct x_bus *bus = (struct x_bus *) dat;
  x_object *o_ = bus->regs.r_obj;

  x_string_putc(&o_->content, c);

  return 0;
}

/* default destructor */
static void
__x_bus_on_release(x_object *o)
{
  ENTER;
  EXIT;
}

/**
 * @todo Make this static and add reset callback
 * to x_bus structure
 */EXPORT_SYMBOL void
x_bus_reset(struct x_bus *bus)
{
  x_object *streamo;

  ENTER;

  if (!bus)
    return;

  bus->regs.r_obj = X_OBJECT(bus);
  __x_bus_set_state(bus, BUS_STATE_INI);

  /* reset parser */
  bus->p->r_reset(bus->p);

  _REFPUT(_CHLD(X_OBJECT(bus), "stream"), NULL);

  /* append stream object */
  streamo = _NEW("stream", "http://etherx.jabber.org/streams");
  _ASET(streamo, "remote",
      _AGET((x_object *) bus, "domain"));

  _ASET(streamo, "local", _AGET((x_object *) bus, "jid"));
  _INS(X_OBJECT(bus), streamo);

  __x_bus_set_state(bus, BUS_STATE_STREAMING);

  EXIT;
}

/**
 * Main event loop
 */
static void
__x_bus_start(struct x_bus *bus)
{
  int err;
  ENTER;

  x_object_set_name(X_OBJECT(bus), x_object_getattr(X_OBJECT(bus), "hostname"));

  err = __x_bus_connect(bus);

  if (err)
    {
      TRACE("ERROR! Connection failed!");
      return;
    }

  TRACE("Registering watcher on socket with fd=%d\n", bus->sys_socket);

  ev_io_init(&bus->io_watcher, &__x_bus_on_signal_data, bus->sys_socket,
      EV_READ /* | EV_WRITE */);

  ev_io_start(bus->obj.loop, &bus->io_watcher);

  x_bus_reset(bus);

#pragma message("THIS SHOULD BE DONE IN OBJECT (BUS) EVENT HANDLER")

  x_path_listeners_notify("/", &bus->obj);

  TRACE("Start handling events\n");
#if 0
  ev_run(bus->loop, 0);
#else
  ev_loop(bus->obj.loop, 0);
#endif
}

/*
 * This handles finalize call
 * and starts up bus.
 */
static void
__x_bus_finalize(x_object *this__)
{
  struct x_bus *bus = (struct x_bus *) this__;
  ENTER;

  TRACE("\n");

//  signal(SIGPIPE, SIG_IGN);

  /* first commit changes */
  __x_bus_jid_commit(this__);

  TRACE("\n");
  /* start the bus */
  __x_bus_start(bus);
  TRACE("\n");

  EXIT;
}

/**
 * Xbus class declaration
 */
static struct xobjectclass xbus_class;

__x_plugin_visibility
__plugin_init void
xbus_lib_init(void)
{
  ENTER;

  xbus_class.cname = "#xbus";
  xbus_class.psize = (unsigned int) (sizeof(struct x_bus) - sizeof(x_object));
  xbus_class.tx = &x_bus_tx;
  xbus_class.on_create = &__x_bus_on_create;
  xbus_class.on_release = &__x_bus_on_release;
  xbus_class.on_assign = &x_object_default_assign_cb;
  xbus_class.finalize = __x_bus_finalize;

  x_class_register_ns(xbus_class.cname, &xbus_class, "gobee");
  EXIT;
  return;
}
PLUGIN_INIT(xbus_lib_init)
;

/*///////////////////////////////////////*/
/*///////////////////////////////////////*/
/*///////////// XBUS API ////////////////*/
/*///////////////////////////////////////*/
/*///////////////////////////////////////*/
void
x_bus_set_transport(struct x_bus *bus, struct x_transport *t)
{
  bus->transport = t;
}

int
x_bus_write(struct x_bus *bus, void *buf, size_t len, int flags)
{
  int err;
  TRACE("\n'%s'\n", (char *)buf);
#ifdef LOG_XMPPP
  write(bus->xmppoutfd, buf, len);
#endif
  err = bus->transport->wr(bus->transport, buf, len, flags);
  return err;
}
