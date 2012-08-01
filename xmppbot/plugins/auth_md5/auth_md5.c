/*
 * auth_digest_md5.c
 *
 *  Created on: Aug 17, 2010
 *      Author: artur
 */

#include <string.h>
#include <sys/types.h>
// #include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define DEBUG_PRFX "[auth-DIGEST-MD5] "
#include <x_types.h>
#include <x_obj.h>
#include <x_utils.h>
#include <xmppagent.h>

#include <crypt/md5.h>
#include <crypt/base64.h>

struct md5_sess
{
  int state;
  char *realm;
  char *nonce;
  char *cnonce;
  char *qop;
};

/**
 *
 */
static void
md5_sess_free(struct md5_sess *ms)
{
  if (!ms)
    return;
  if (ms->realm != NULL)
    free(ms->realm);
  if (ms->nonce != NULL)
    free(ms->nonce);
  if (ms->qop != NULL)
    free(ms->qop);
  free(ms);
}

/**
 *
 */
static struct md5_sess *
md5_sess_new_from_string(char *str)
{
  struct md5_sess * mds = NULL;
  char *p1, *p2, *p3, *p4;

  if (!str)
    return NULL;

  // take realm
  p1 = strstr(str, "realm=");
  if (p1)
    p1 += strlen("realm=");

  // take nonce
  p2 = strstr(str, "nonce=");
  if (!p2)
    return NULL;
  p2 += strlen("nonce=");

  // take nonce
  p3 = strstr(str, "qop=");
  if (p3)
    p3 += strlen("qop=");

  mds = (struct md5_sess *) calloc(sizeof(struct md5_sess), 1);

  // format/copy realm
  if (p1)
    {
      p1 += strspn(p1, "\t \n\r\"");
      p4 = p1;
      while (*p4 != '"' && *p4 != ',')
        p4++;
      *p4 = '\0';
      mds->realm = x_strdup(p1);
    }
  else
    mds->realm = NULL;

  // format/copy nonce
  p2 += strspn(p2, "\t \n\r\"");
  p4 = p2;
  while (*p4 != '"' && *p4 != ',')
    p4++;
  *p4 = '\0';
  mds->nonce = x_strdup(p2);

  if (p3)
    {
      // format/copy qop
      p3 += strspn(p3, "\t \n\r\"");
      p4 = p3;
      while (*p4 != '"' && *p4 != ',')
        p4++;
      *p4 = '\0';
      mds->qop = x_strdup(p3);
    }

  /* TODO Generate cnonce randomly */
  mds->cnonce = "UrRqNwb8BU+JqpmnhoM=";
  mds->state = 0; // initial state

  return mds;
}

#if 0
  {
    md5_state_t state;
    md5_byte_t digest[16];
    char hex_output[16*2 + 1];
    int di;

    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)test[i], strlen(test[i]));
    md5_finish(&state, digest);
  }
#endif

/**
 * calculates md5 hash of string
 */
static
char *
get_md5(char *str, int len)
{
  int i;
  md5_state_t state;
  md5_byte_t digest[16];
  char hex[33];

  md5_init(&state);
  md5_append(&state, (const md5_byte_t *) str, len);
  md5_finish(&state, digest);

  for (i = 0; i < 16; ++i)
    {
      snprintf(hex + i * 2, 3, "%02x", digest[i]);
    }

  return x_strdup(hex);
}

/**
 * return A1 string
 */
static char *
get_HA1(struct md5_sess *ms, const char *uname, const char *pw, const char *jid)
{
  char *str, *ret;
  md5_state_t state;
  md5_byte_t digest[16];

  if (!ms)
    return NULL;

  /* HA1 = MD5 (A1 = "username:realm:passwd") */
  str = malloc(strlen(uname) + strlen(ms->realm) + strlen(pw) + 3);
  sprintf(str, "%s:%s:%s", uname, ms->realm, pw);

  //	printf("\n======= %s ========\n",str);

  md5_init(&state);
  md5_append(&state, (const md5_byte_t *) str, strlen(str));
  md5_finish(&state, digest);

  str = realloc(str, 16 + strlen(ms->nonce) + strlen(ms->cnonce) + strlen(jid)
      + 4);
  memcpy(str, digest, 16);
  sprintf(str + 16, ":%s:%s:%s", ms->nonce, ms->cnonce, jid);

  printf("\n======= %s ========\n", str);

  // A1 = { H ( {user-name : realm-value : passwd } ),
  // : nonce-value, : cnonce-value : authzid-value
  ret = get_md5(str, 16 + strlen(ms->nonce) + strlen(ms->cnonce) + 2);

  if (str)
    free(str);

  return ret;
}

/**
 * TODO Check for thread safety!
 */
static char *
get_HA2(const char *realm)
{
  char *str, *ret;

  /* Not JIDDOMAIN info */
  if (!realm)
    return NULL;

  /* HA1 = MD5 (A1 = "username:realm:passwd") */
  str = malloc(strlen("AUTHENTICATE:xmpp/") + strlen(realm) + 1);

  /* TODO Move string to dictionary */
  sprintf(str, "AUTHENTICATE:xmpp/%s", realm);

  ret = get_md5(str, strlen(str));
  if (str)
    free(str);
  return ret;
}

/**
 * first
 */
static char *
rspauth(struct md5_sess **mds, char *b64str, const char *uname, const char *pw,
    const char *jid, const char *jiddomain)
{
  char *str, *str1 = NULL, *str2;
  struct md5_sess *ms;
  char * rsp = 0;

  if (!mds)
    return NULL;
  ms = *mds;

  // send md5hash
  str = base64_decode(b64str, strlen(b64str), NULL);
  ;
  if (str)
    rsp = strstr(str, "rspauth");

  TRACE("SERVER RESPONSE:\n%s => %s\n", b64str, str);

  /*
   * FIXME Remove following condition!!!!
   */
  if (rsp)
    {
      str1 = x_strdup("");
      goto out;
    }

  // create new session if no one
  if (!ms)
    ms = md5_sess_new_from_string(str);

  // check realm presence
  // calc default realm if needed
  if (NULL == ms->realm)
    {
      TRACE("USING DEFAULT REALM: %s\n", jiddomain);
      ms->realm = x_strdup(jiddomain);
    }

  /* clenaup as soon as possible */
  if (str)
    free(str);

  if (ms)
    {
      if (ms->state > 0 && rsp)
        {
          ms->state++;
          str1 = x_strdup("");
          goto out;
        }

      str1 = get_HA1(ms, uname, pw, jid);
      //		printf(" HA1 = (%s)\n",str1);
      str2 = get_HA2(jiddomain);
      //		printf(" HA2 = (%s)\n",str2);
      str = malloc(1024);
#ifndef RFC_2069
      sprintf(str, "%s:%s:00000001:%s:%s:%s", str1, ms->nonce, ms->cnonce,
          ms->qop, str2);
#else
      sprintf(str,"%s:%s:%s",
          str1,ms->nonce,str2);
#endif

      if (str1)
        {
          free(str1);
          str1 = NULL;
        }
      if (str2)
        {
          free(str2);
          str2 = NULL;
        }

      str1 = get_md5(str, strlen(str));
      //		printf("\nRESPONSE = (%s)\n",str);
      //		printf(" MD5 (RESPONSE) = (%s)\n",str1);

      sprintf(str, "username=\"%s\",realm=\"%s\",nonce=\"%s\",cnonce=\"%s\","
        "nc=00000001,qop=auth,digest-uri=\"xmpp/%s\",response=%s,"
        "charset=utf-8", uname, ms->realm, ms->nonce, ms->cnonce, jiddomain,
          str1);

      TRACE("\n\n%s\n\n", str);

      if (str1)
        free(str1);
      str1 = x_base64_encode(str, strlen(str));

      if (str)
        free(str);
      //		printf(" RESPONSE = (%s)\n",str1);

      ms->state++;
    }

  out: return str1;
}

/* matching function */
static int
auth_digest_md5_match(x_object *o, x_obj_attr_t *_o)
{
  const char *a;
  ENTER;

  if (_o && (a = getattr("xmlns", _o)) && !EQ(a,
      "urn:ietf:params:xml:ns:xmpp-sasl"))
    return FALSE;

  EXIT;
  return TRUE; // by default will treat any 'starttls' or 'proceed' as matched
}

static int
auth_digest_md5_on_append(x_object *me, x_object *parent)
{
  x_object *msg;

  ENTER;

  /* prepare auth stanza */
  msg = x_object_new("auth");
  x_object_setattr(msg, "xmlns", "urn:ietf:params:xml:ns:xmpp-sasl");
  x_object_setattr(msg, "mechanism", "DIGEST-MD5");

  x_object_send_down(parent, msg, NULL);

  EXIT;
  return 0;
}

static void
auth_digest_md5_exit(x_object *this__)
{
  char *str;
  x_object *msg = NULL;

  struct md5_sess *ms = NULL;
  const char *unam;
  const char *pw;
  const char *jid;
  const char *jiddomain;
  const char *hostname;

  ENTER;

  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);

  if (EQ(x_object_get_name(this__), "challenge"))
    {
      /* take session params */
      unam = x_object_getenv(this__, "username");
      pw = x_object_getenv(this__, "pword");
      jid = x_object_getenv(this__, "jid");
      jiddomain = x_object_getenv(this__, "domain");
      hostname = x_object_getenv(this__, "hostname");

      str = rspauth(&ms, this__->content.cbuf, unam, pw, jid, jiddomain);
      msg = x_object_new("response");
      x_object_setattr(msg, "xmlns", "urn:ietf:params:xml:ns:xmpp-sasl");

      x_string_write(&msg->content, str, x_strlen(str));

      x_object_send_down(x_object_get_parent(this__), msg, NULL);

      if (str)
        free(str);

      if (ms != NULL)
        md5_sess_free(ms);
    }

  EXIT;
}

static struct xobjectclass auth_digest_md5_class =
  { .cname = "auth-digest-md5", .psize = 0, .match = &auth_digest_md5_match,
      .on_append = &auth_digest_md5_on_append,
      .finalize = &auth_digest_md5_exit, };

/**
 *
 */
static void
auth_digest_md5_register(void)
{
  x_class_register(auth_digest_md5_class.cname, &auth_digest_md5_class);
  x_class_register("challenge", &auth_digest_md5_class);
  return;
}
PLUGIN_INIT(auth_digest_md5_register)
;
