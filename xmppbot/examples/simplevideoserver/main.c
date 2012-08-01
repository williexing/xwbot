/**
 * main.c
 *
 *  Created on: Jan 11, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[ videoSender ] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>
#include <ev.h>

#ifndef WIN32
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <dirent.h>
#include <dlfcn.h>
#endif

#include "mynet.h"

static x_object *session1, *session2;
static x_object *xroot;
static char *stunsrv = NULL;

/**
 * Load shared plugins
 */
#ifdef WIN32
static void gobee_load_plugins (const char *_dir)
{
	HMODULE hDll;
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;

	TRACE("Loading plugins from '%s' ....\n",_dir);

	if((hFind = FindFirstFile("*.dll", &FindFileData)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			printf("%s\n", FindFileData.cFileName);
			if ((hDll = LoadLibrary(FindFileData.cFileName)) != 0)
			{
				TRACE("OK! loaded %s\n", FindFileData.cFileName);
			}
			else
			{
				TRACE("ERROR loading plugin: %s...\n", FindFileData.cFileName);
			}
		} 
		while (FindNextFile(hFind, &FindFileData));

		FindClose(hFind);
	}
}
#else /* WIN32 */
static void
gobee_load_plugins(const char *_dir)
{
  char libname[64];
  DIR *dd;
  struct dirent *de;
  char *dir = (char *) _dir;

  TRACE("Loading plugins from '%s' ....\n",_dir);

  if ((dd = opendir(dir)) == NULL)
    {
      TRACE("ERROR");
      return;
    }

  while ((de = readdir(dd)))
    {
      snprintf(libname, sizeof(libname), "%s/%s", dir, de->d_name);
      if (!dlopen(libname, RTLD_LAZY | RTLD_GLOBAL))
        {
          TRACE("ERROR loading plugins: %s...\n", dlerror());
        }
      else
        TRACE("OK! loaded %s\n", libname);
    }
}
#endif /* WIN32 */

static void
hexdump(char *buf, int len)
{
  int i;
  for (i = 0; i < len; ++i)
    {
      printf("%02x", ((unsigned char) buf[i] & 0xff));
    }
  putchar('\n');
}

static void
asciidump(char *buf, int len)
{
  int i;
  for (i = 0; i < len; ++i)
    {
      putchar((int) (buf[i] & 0xff));
    }
  putchar('\n');
}

static char *
gobee_parse_dns_srv_reply(unsigned char *reply, int len)
{
  unsigned char *entry;
  int i = 12;
  int k;
  uint32_t *TTL;
  uint16_t *answer_type;
  uint16_t *prio;
  uint16_t *weight;
  uint16_t *portnum;

  if (!len)
	  return NULL;

  entry = &reply[i + 1];
  do
    {
      k = reply[i];
      /*      printf("%d(%x)\n", k, k);*/
      reply[i] = (unsigned char) '.';
      i += k + 1;
    }
  while (reply[i]);
  i++;

  TRACE("'%s' ok...\n", entry);

  // skip QTYPE
  i += 2;
  // skip QCLASS
  i += 2;

  answer_type = (uint16_t *) &reply[i];
  while (*answer_type == 0x0cc0)
    {
      i += 2;

      // skip DNS type code (SRV)
      i += 2;

      // skip QCLASS
      i += 2;

      // skip TTL
      TTL = (uint32_t *) &reply[i];
      i += 4;

      // skip something?????????????????????????
      i += 2;

      // skip priority
      prio = (uint16_t *) &reply[i];
      i += 2;
      // skip weight
      weight = (uint16_t *) &reply[i];
      i += 2;

      portnum = (uint16_t *) &reply[i];
      i += 2;

      /* first entry */
      entry = &reply[i + 1];
      do
        {
          k = reply[i];
          reply[i] = (unsigned char) '.';
          i += k + 1;
        }
      while (reply[i]);
      i++;
      TRACE("'%s' TTL=%d %d %d %d\n",entry,ntohl(*TTL),
          ntohs(*prio),ntohs(*weight),ntohs(*portnum));

      answer_type = (uint16_t *) &reply[i];
    }

  return x_strdup((const char *) entry);
}

static char *
gobee_resolv_stun_server(const char *srvname, const char *domainname)
{
  int len = 0;
  unsigned char stun_srv_reply[512];
  char stun_srv_query[512];

  sprintf(stun_srv_query, "%s.%s", srvname, domainname);

#ifndef C_IN
#	ifdef 	ns_class
#		define C_IN ns_c_in
#	else
#		define C_IN 1
#	endif
#endif

#ifndef T_SRV
#	ifdef ns_typw
#		define T_SRV ns_t_srv
#	else
#		define T_SRV 33
#	endif
#endif

#ifndef PACKETSZ
#define PACKETSZ 512
#endif

#ifndef WIN32
  if ((len = res_query(stun_srv_query, C_IN, T_SRV, &stun_srv_reply[0],
      PACKETSZ)) < 0)
    {
      TRACE("@Can't query %s\n", stun_srv_query);
      return NULL;
    }
#endif

  /*
   hexdump(&stun_srv_reply[12], len - 12);
   asciidump(&stun_srv_reply[12], len - 12);
   printf("%s\n", &stun_srv_reply[12]);
   */
  return gobee_parse_dns_srv_reply(&stun_srv_reply[0], len);
}

/*-----------------------------------
 *-----------------------------------
 *-----------------------------------
 *-----------------------------------*/

static int
__mybus_classmatch(UNUSED x_object *to, x_obj_attr_t *attr)
{
  ENTER;
  EXIT;
  return 1;
}

static BOOL
__mybus_equals(x_object *o, x_obj_attr_t *attrs)
{
  ENTER;
  EXIT;
  return TRUE;
}

static void UNUSED
__mybus_on_child_append(x_object *o, x_object *child)
{
  ENTER;
  TRACE("Appended '%s'\n",x_object_get_name(child));
  EXIT;
}

static void
__recv_msg_object(x_object *to, x_object *o)
{
  x_object *nexthop;
  x_object *child;
  x_object *curobj = NULL;
  const char *oname = x_object_get_name(o);

  ENTER;

  curobj = to;

  // x_object_print_path(o, 0);

  if ((child = x_object_find_child(curobj, &o->attrs)) != NULL)
    {
      TRACE("Object '%s' FOUND\n",oname);
      goto objopen_end;
    }

  TRACE("Object '%s' not FOUND\n",oname);

  /* instantiate object from class */
  if (!(child = x_object_new_ns_match(oname, NULL, &o->attrs, TRUE)))
    {
      TRACE("Error creating '%s' object\n",oname);
      return;
    }

  /* Append child object */
  if (x_object_append_child(curobj, child))
    {
      TRACE("ERROR!");
      return;
    }

  objopen_end: curobj = child;

  x_object_assign(child, &o->attrs);

  nexthop = x_object_get_child(o, NULL);
  TRACE("Receiving child objects [cur=%p,%p],nexthop=%p\n",
      curobj,child,nexthop);

  for (; nexthop; nexthop = x_object_get_sibling(nexthop))
    {
      __recv_msg_object(curobj, nexthop);
    }

  //  x_object_print_path(to, 0);

  EXIT;
}

static int
__mybus_tx(x_object *to, x_object *o, x_obj_attr_t *ctx_attrs)
{
  const char *from, *tto;
  char buf[X_LIB_BUFLEN];
  ENTER;

  from = x_object_getattr(o, X_STRING("from"));
  tto = x_object_getattr(o, X_STRING("to"));

  x_snprintf(buf, sizeof(buf), "%s", from);

  x_object_setattr(o, X_STRING("from"), tto);
  x_object_setattr(o, X_STRING("to"), buf);
  //  x_object_print_path(o, 0);

  __recv_msg_object(to, o);

  x_object_destroy_no_cb(o);

  EXIT;
  return 0;
}

static struct xobjectclass __my_test_bus_objclass;

static __plugin_init void
	__my_test_bus_init(void)
{
	ENTER;

	__my_test_bus_objclass.cname = "__my_test_bus";
	__my_test_bus_objclass .psize = 0;
	__my_test_bus_objclass .match = &__mybus_equals;
	__my_test_bus_objclass .classmatch = &__mybus_classmatch;
	__my_test_bus_objclass .on_child_append = &__mybus_on_child_append;
	__my_test_bus_objclass .tx = &__mybus_tx;

	x_class_register(__my_test_bus_objclass.cname, &__my_test_bus_objclass);
	EXIT;
	return;
}

PLUGIN_INIT(__my_test_bus_init)
	;


/*-----------------------------------
 *-----------------------------------
 *-----------------------------------
 *-----------------------------------*/
static void
__shell_parse_attrs(const char *astr, x_obj_attr_t *p_attrs)
{
  x_string key;
  x_string val;
  x_string *scur = &key;

  key.capacity= 0;
  key.cbuf = 0;
  key.pos=0;
  val.capacity= 0;
  val.cbuf = 0;
  val.pos=0;

  while (x_isspace((int) (*astr)))
    astr++;

  for (; *astr; astr++)
    {
      if (*astr == '=' && scur == &key)
        {
          scur = &val;
          continue;
        }
      else if ((!(*astr) || x_isspace(*astr)) && (scur == &val))
        {
          printf(">>> %s:%s\n", key.cbuf, val.cbuf);
          if (val.pos)
            {
              printf("*** %s:%s\n", key.cbuf, val.cbuf);
              setattr(key.cbuf, val.cbuf, p_attrs);
              printf("%s:%s\n", key.cbuf, val.cbuf);
            }
          x_string_clear(&key);
          x_string_clear(&val);
          scur = &key;
          while (x_isspace((int) (*astr)))
            astr++;
          if (*astr)
            --astr;
          continue;
        }
      else
        {
          x_string_putc(scur, *astr);
        }
    }

  x_string_trunc(&key);
  x_string_trunc(&val);
  return;
}

static void
__shell_assign(x_object *curobj, const char *cmdstr)
{
  char _path[256];
  char _attrs[382];
  char *p;
  int len;
  x_obj_attr_t *attr;
  x_obj_attr_t attrs =
    { 0, 0, 0 };
  x_object *xobj;

  memset(&_path[0], 0, sizeof(_path));

  len = strcspn(cmdstr, "\t\n\ \r\v\f");
  x_strncpy(_path, cmdstr, len);
  p = cmdstr + len;
  while (x_isspace((int) (*p)))
    p++;

  x_strncpy(_attrs, p, sizeof(_attrs));
  printf("Assigning '%s' to '%s' at %p\n", _attrs, _path, curobj);

  xobj = x_object_from_path(curobj, _path);
  if (!xobj)
    return;

  __shell_parse_attrs(_attrs, &attrs);

  for (attr = attrs.next; attr; attr = attr->next)
    {
      printf("+++ %s:%s\n", attr->key, attr->val);
    }

  x_object_assign(xobj, &attrs);

  return;
}

static int
__shell_parse_cmd(char *cmdbuf, int nbytes)
{
  int is_exit = FALSE;
  char *p = cmdbuf;
  x_object *tmpo = NULL;

  while (x_isspace((int) (*p)))
    p++;

  if (strstr(p, "ls") == p)
    {
      p += 2;
      // skip whitespaces
      while (x_isspace((int) (*p)))
        p++;

      printf("PATH: %s\n", p);

      tmpo = x_object_from_path(xroot, p);
      if (tmpo)
        x_object_print_path(tmpo, 0);
    }
  else if (strstr(p, "quit") == p)
    {
      is_exit = TRUE;
    }
  else if (strstr(p, "load") == p)
    {
      p += 4;
      // skip whitespaces
      while (x_isspace((int) (*p)))
        p++;
      tmpo = x_object_new_ns(X_STRING(p), X_STRING("jingle"));
      x_object_append_child(xroot, tmpo);
    }
  else if (strstr(p, "assign") == p)
    {
      p += 6;
      // skip whitespaces
      while (x_isspace((int) (*p)))
        p++;
      __shell_assign(xroot, p);
    }
  return is_exit;
}

#ifdef WIN32
#include <tchar.h>
int _tmain (int argc, TCHAR *argv[])
#else
int main (int argc, char *argv[])
#endif
{
  int c;
  int err;

  x_string cmdbuf =
    { 0, 0, 0 };
  x_obj_attr_t attrs =
    { 0, 0, 0 };

  x_object *chan;

  /* load all additional plugins from plugins dir */
  gobee_load_plugins(".");

  /* start networking */
#ifdef _WIN32
  {
	struct ev_loop *loop;
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
	wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    BUG_ON(err);
	loop = ev_default_loop(EVFLAG_AUTO);
  }
#endif

  /* discover stun server for domain */
  if (argv[1])
    {
      stunsrv = gobee_resolv_stun_server("_stun._udp", argv[1]);
    }

  xroot = x_object_new_ns("__my_test_bus", "jingle");
  BUG_ON (!xroot);
  /* xroot is a bus */
  xroot->bus = xroot;

  /*
   * Create new session object and set its attributes
   * and append it to root object.
   */
    {
      session1 = x_object_new_ns(X_STRING("__isession"), X_STRING("jingle"));
      BUG_ON (!session1);

      x_object_append_child(xroot, session1);

      /* set session params */
      setattr(X_STRING("sid"), X_STRING("1234567890"), &attrs);
      setattr(X_STRING("stunserver"), stunsrv, &attrs);
      setattr(X_STRING("ufrag"), X_STRING("romeo"), &attrs);
      setattr(X_STRING("pwd"), X_STRING("montekchi"), &attrs);
      setattr(X_STRING("otherend-ufrag"), X_STRING("juliet"), &attrs);
      setattr(X_STRING("otherend-pwd"), X_STRING("capulet"), &attrs);
      setattr(X_STRING("from"), X_STRING("sess1"), &attrs);
      setattr(X_STRING("to"), X_STRING("sess2"), &attrs);
      x_object_assign(session1, &attrs);

    }

  attr_list_clear(&attrs);

    {
      chan = x_object_new_ns(X_STRING("__iostream"), X_STRING("jingle"));
      BUG_ON (!chan);

      x_object_append_child(session1, chan);

      setattr(X_STRING("transport-profile"), X_STRING("__icectl"), &attrs);
      setattr(X_STRING("name"), X_STRING("video"), &attrs);
      setattr(X_STRING("creator"), X_STRING("initiator"), &attrs);
      x_object_assign(chan, &attrs);
    }

  attr_list_clear(&attrs);

#if 0
    {
      session2 = x_object_new_ns("__session", "jingle");
      BUG_ON (!session2);

      x_object_set_name(session2, X_STRING("sess2"));
      x_object_append_child(xroot, session2);

      /* set session params */
      setattr("stunserver", stunsrv, &attrs);
      setattr("ufrag", "juliet", &attrs);
      setattr("pwd", "capulet", &attrs);
      setattr("otherend-ufrag", "romeo", &attrs);
      setattr("otherend-pwd", "montekchi", &attrs);
      setattr(X_STRING("otherend"), X_STRING("sess1"), &attrs);

      x_object_assign(session2, &attrs);
    }

  attr_list_clear(&attrs);

    {
      chan = x_object_new_ns(X_STRING("__iostream"), X_STRING("jingle"));
      BUG_ON (!chan);

      x_object_append_child(session2, chan);

      setattr(X_STRING("transport-profile"), X_STRING("__icectl"), &attrs);
      setattr(X_STRING("name"), X_STRING("video"), &attrs);
      setattr(X_STRING("creator"), X_STRING("initiator"), &attrs);
      x_object_assign(chan, &attrs);
    }

  attr_list_clear(&attrs);
#endif

 for (;;)
  {
      printf("xw@:__my_test_bus$ ");
      do
        {
          c = getchar();
          x_string_putc(&cmdbuf, (char) (c & 0xff));
        }
      while (c != '\n');
      printf(">");
      err = __shell_parse_cmd(cmdbuf.cbuf, cmdbuf.pos);
      if (err > 0)
        {
          x_string_trunc(&cmdbuf);
          break;
        }
      x_string_clear(&cmdbuf);
    }

  _REFPUT(xroot,NULL);

  return 0;
}

