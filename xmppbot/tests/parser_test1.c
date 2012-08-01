/*
 * parser_test1.c
 *
 *  Created on: Aug 7, 2011
 *      Author: artemka
 */

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>

#include <openssl/ssl.h>

#include <x_utils.h>
#include <x_obj.h>
#include <x_string.h>
#include <x_parser.h>
#include <xmppagent.h>

static struct x_bus xbus;

static int
__x_bus_objopen(const char *name, struct ht_cell *attrs, void *dat)
{
  x_object *o_;
  struct x_bus *bus = (struct x_bus *) dat;

  /* first try existing objects */
  for (o_ = x_object_get_child(bus->regs.r_obj, name); o_; o_
      = x_object_get_next(o_))
    if (x_object_match(o_, attrs))
      goto objopen_end;

  /* instantiate object from class */
  if (!(o_ = x_object_new(name)))
    {
      TRACE("Error creating '%s' object\n",name);
      return -1;
    }
  /*
   else
   {
   // save bus reference for every object
   o_->bus = dat;
   }
   */

  /* Append child object */
  if (x_object_append_child(bus->regs.r_obj, o_))
    {
      TRACE("ERROR!");
      return -1;
    }

  objopen_end: bus->regs.r_obj = o_;

  x_object_assign(o_, attrs);

  return 0;
}

/**
 * Closes object returns its parent
 */
static int
__x_bus_closeobj(const char *name, void *dat)
{
  struct x_bus *bus = (struct x_bus *) dat;
  x_object *o_ = bus->regs.r_obj;

  if (strcasecmp(name, x_object_get_name(o_)))
    {
      /* TODO */
#warning "TODO Finish this!!!"
      return -1;
    }

  bus->regs.r_obj = x_object_get_parent(o_);

  x_object_lost_focus(o_);

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

void
string_test(x_string *s)
{
  int i;
  memset(s, 0, sizeof(*s));
  for (i = 0; i < 10000; i++)
    {
      x_string_putc(s, 'c');
      printf("string size=%d, i=%d\n", s->capacity, s->pos);
    }
}

struct ssl_transport
{
  struct x_transport super;
  SSL *ssl;
  SSL_CTX *ctx;
};

static int
initialize_ctx(struct ssl_transport *sslt)
{
  SSL_load_error_strings();
  SSL_library_init();

  sslt->ctx = SSL_CTX_new(SSLv23_client_method());

  if (sslt->ctx == NULL)
    {
      printf("SSL Error\n");
      return -1;
    }
  else
    printf("SSL Ok\n");

  // SSL_CTX_set_options(sess->ssl_ctx, SSL_OP_NO_SSLv2);

  sslt->ssl = SSL_new(sslt->ctx);

  printf("New Ssl created\n");

  return 0;
}

int
main(int argc, char **argv)
{
  char volatile c;
  x_string xstr =
  X_STRING_STATIC_INIT;

  struct ssl_transport *sslt = calloc(1, sizeof(struct ssl_transport));
  initialize_ctx(sslt);

  // FIXME: remove this bitch!
  // ht_testtable();

  // FIXME: remove this bitch!
  //  xobject_testser();

  int fd = open(argv[1], O_RDONLY);
  int fd2 = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR
      | S_IRGRP | S_IROTH);

  x_parser_init(&xbus.p);
  xbus.regs.r_obj = (x_object *) &xbus;
  xbus.regs.r_obj->bus = (x_object *) &xbus;
  TREE_NODE(&xbus.obj.entry)->name = "bus";

  xbus.p.xobjopen = &__x_bus_objopen;
  xbus.p.xobjclose = &__x_bus_closeobj;
  xbus.p.xputchar = &__x_bus_putchar;
  xbus.p.cbdata = &xbus;

  for (; read(fd, (void *) &c, 1);)
    {
      xbus.p.r_putc(&xbus.p, c);
    }

  x_string_clear(&xstr);

  x_parser_object_to_fd((x_object *) &xbus, fd2);
  //  x_object_to_string((x_object *) &xbus, &xstr);
  //write(fd2, xstr.cbuf, xstr.pos);

  return 0;
}

