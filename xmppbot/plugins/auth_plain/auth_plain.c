/*
 * auth_plain.c
 *
 *  Created on: Aug 6, 2010
 *      Author: artur
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
/* #include <sys/socket.h> */

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_XMPPAUTH_ON
#define DEBUG_PRFX "[auth-plain] "
#endif
#include <xwlib/x_types.h>
#include <xwlib/x_lib.h>
#include <xmppagent.h>

#include <crypt/base64.h>

/**
 *
 */
static char *
xmpp_auth_get_plain_hash(const char *uname, const char *pw,
    const char *jiddomain)
{
  // jid . chr(0) . login . chr(0) . password
  char *tmp = 0;
  char *ret = 0;
  int ulen = 0;
  int hlen = 0;
  int plen = 0;

  ulen = x_strlen(uname);
  hlen = x_strlen(jiddomain);
  plen = x_strlen(pw);

  tmp = (char *) x_local_malloc(2 * ulen + hlen + plen + 3);

  sprintf(tmp, "%s@%s", uname, jiddomain);
  x_strcpy(tmp + ulen + hlen + 2, uname);
  x_strcpy(tmp + 2 * ulen + hlen + 3, pw);
  ret = x_base64_encode(tmp, 2 * ulen + hlen + plen + 3);

  if (tmp)
    x_free(tmp);

  return ret;
}

/* matching function */
static int
auth_plain_match(x_object *o, x_obj_attr_t *_o)
{
  const char *a;
  ENTER;

  if (_o && (a = getattr("xmlns", _o))
      && !EQ(a,"urn:ietf:params:xml:ns:xmpp-sasl"))
    return FALSE;

  EXIT;
  return TRUE; // by default will treat any 'starttls' or 'proceed' as matched
}

static int
auth_plain_on_append(x_object *me, x_object *parent)
{
  char *str = NULL;
  x_object *msg;
  const char *unam;
  const char *pw;
  const char *jid;
  const char *jiddomain;
  const char *hostname;

  ENTER;

  /* take session params */
  unam = x_object_getenv(me, "username");
  pw = x_object_getenv(me, "pword");
  jid = x_object_getenv(me, "jid");
  jiddomain = x_object_getenv(me, "domain");
  hostname = x_object_getenv(me, "hostname");

  /* prepare auth stanza */
  str = xmpp_auth_get_plain_hash(unam, pw, jiddomain);
  TRACE("Created new sasl string [%s:%s:%s]:\n%s, parent='%s'\n",
        unam, pw, jiddomain,str,_GETNM(parent));

  msg = _GNEW("$message", NULL);
  _SETNM(msg, "auth");
  x_object_setattr(msg, "xmlns", "urn:ietf:params:xml:ns:xmpp-sasl");
  x_object_setattr(msg, "mechanism", "PLAIN");
  x_string_write(&msg->content, str, x_strlen(str));
  if (str)
    x_free(str);

  x_object_send_down(parent, msg, NULL);
//  _REFPUT(msg, NULL);
  
  EXIT;
  return 0;
}

static void
auth_plain_exit(x_object *o)
{
  ENTER;
  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
  EXIT;
}

static struct xobjectclass auth_plain_class;

/**
 *
 */
__x_plugin_visibility __plugin_init void
	auth_plain_register(void)
{
	auth_plain_class.cname = "auth-plain";
	auth_plain_class.psize = 0;
	auth_plain_class.match = &auth_plain_match;
	auth_plain_class.on_append = &auth_plain_on_append;
	auth_plain_class.commit = &auth_plain_exit;
	x_class_register(auth_plain_class.cname, &auth_plain_class);
	return;
}
PLUGIN_INIT(auth_plain_register)
	;

