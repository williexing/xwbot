/*
 * shell.c
 *
 *  Created on: Jul 28, 2011
 *      Author: artemka
 */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <xmppagent.h>

// shell job
int shell_out[2];
int shell_in[2];
pid_t c_pid = 0; // child pid
fd_set shellfds;
char *shell_from;

/**
 * Decodes 'src' string of content
 * and replaces special html tags bac to
 * characters.
 * \return same decoded string
 * \param 'src' '\0' terminated source string.
 */
static char *
xmpp_content_decode(const char *src)
{

  int l;
  char *dst;

  ENTER;

  if (src == NULL || !(l = strlen(src)))
    {
      EXIT;
      return NULL;
    }

  dst = (char *) src;

  while (*src)
    {
      if (*src == '&')
        {
          if (!strcmp(src, "&amp;"))
            {
              *dst++ = '&';
              src += 4;
            }
          else if (!strcmp(src, "&gt;"))
            {
              *dst++ = '>';
              src += 3;
            }
          else if (!strcmp(src, "&lt;"))
            {
              *dst++ = '<';
              src += 3;
            }
          else if (!strcmp(src, "&nbsp;"))
            {
              *dst++ = ' ';
              src += 5;
            }
        }
      else
        *dst++ = *src++;
    }

  EXIT;
  return dst;
}

/**
 * TODO Check stanza against special trusted ezxml node
 * in session structure
 */
static int
my_is_stanza_acceptable(struct x_bus *sess, ezxml_t __stnz)
{
  int err = 1;
  char *str, *to;
  const char *myjid;
  const char *jdomain;
  const char *uname;
  ezxml_t acl;

  pthread_mutex_lock(&sess->lock);
  myjid = ezxml_attr(sess->dbcore, "jid");
  uname = ezxml_attr(sess->dbcore, "username");
  jdomain = ezxml_attr(sess->dbcore, "domain");

  str = (char *) ezxml_attr(__stnz, "from");
  to = (char *) ezxml_attr(__stnz, "to");

  /* TODO Possibly this is session specific iq's */
  if (!str && !to)
    {
      err = 0;
      goto _fin;
    }

  if (!strcasecmp(str, myjid))
    {
      TRACE("-ERROR! My own stanza\n");
      err = 1;
      goto _fin;
    }

  /* TODO Need smart acl cheking!!!! */
  /* by default trust all non user jids */
  if (!strchr(str, '@'))
    {
      err = 0;
      goto _fin;
    }

  /* default trustee */
  if (!strncasecmp(str, myjid, strlen(uname) + strlen(jdomain) + 1))
    {
      err = 0;
      goto _fin;
    }

  /* check against access list */
  acl = ezxml_get(sess->dbcore, "acldb", -1);
  if (!acl)
    {
      TRACE("-ERROR! No trustees available\n");
      err = 1;
      goto _fin;
    }

  for (acl = ezxml_child(acl, "node"); acl; acl = ezxml_next(acl))
    {
      if (!strncasecmp(str, ezxml_attr(acl, "jid"), strlen(ezxml_attr(acl,
          "jid"))))
        {
          err = 0;
          goto _fin;
        }
    }

#if 0
  /* FIXME Stream binding must be handled */
  str = (char *)ezxml_attr(__stnz,"to");
  if (str && strcmp(str,myjid))
    {
      TRACE("Not accepting messages for this resource (%s)!\n",str);
      return 0;
    }
#endif

  _fin: TRACE("%s => result:%d\n", str, err);
  pthread_mutex_unlock(&sess->lock);
  ezxml_set_attr(__stnz, "trusted", err ? "no" : "yes");
  return err ? 0 : 1;
}

/**
 *
 */
static int
create_shell_job(struct x_bus *sess, const char *from)
{
  int err;
  ENTER;

  err = pipe(shell_in);
  err = pipe(shell_out);

  /* OK! Now process entire message */
  if (!(c_pid = fork()))
    {
      dup2(shell_out[1], 1);
      dup2(shell_out[1], 2);
      dup2(shell_in[0], 0);

      close(shell_out[0]);
      close(shell_in[1]);

      execl("/bin/sh", "sh", (char*) NULL);
      exit(1);
    }

  close(shell_out[1]);
  close(shell_in[0]);

  fcntl(shell_out[0], F_SETFL, O_NONBLOCK);
  fcntl(shell_in[1], F_SETFL, O_NONBLOCK);

  shell_from = x_strdup(from);
  FD_SET(shell_out[0],&sess->rfds);
  sess->maxfd = sess->maxfd > shell_out[0] ? sess->maxfd : shell_out[0];

  EXIT;
  return 0;
}

/**
 *
 */
static int
my_check_shell(struct x_bus *sess, fd_set *fds)
{
  ezxml_t stnz1, stnz2;
  int l;
  char buf[256];
  const char *myjid;

  ENTER;

  if (!FD_ISSET(shell_out[0], fds))
    {
      EXIT;
      return -1;
    }

  l = read(shell_out[0], buf, sizeof(buf)  -1);
  if (l <= 0)
    return -1;

  buf[l] = 0;

  myjid = ezxml_attr(sess->dbcore, "jid");

  stnz1 = ezxml_new("message");
  ezxml_set_attr(stnz1, "type", "chat");
  ezxml_set_attr(stnz1, "to", shell_from);
  ezxml_set_attr(stnz1, "from", myjid);

  stnz2 = ezxml_add_child(stnz1, "body", 0);
  ezxml_set_txt_d(stnz2,buf);

  xmpp_session_send_stanza(sess, stnz1);

  if (stnz1)
    ezxml_free(stnz1);

  EXIT;
  return 0;
}

/**
 *
 */
static void
my_on_message(struct x_bus *sess, ezxml_t __stnz)
{
  int err;
  ezxml_t _stnz;
  char *str = NULL;

  ENTER;

  if (__stnz == NULL)
    {
      TRACE("Invalid stanza\n");
      return;
    }

  if (!my_is_stanza_acceptable(sess, __stnz))
    return;

  str = (char *) ezxml_attr(__stnz, "from");

  // replace origin
  if (shell_from && strcmp(shell_from, str))
    {
      free(shell_from);
      shell_from = x_strdup(str);
    }

  /* Take a body */
  _stnz = ezxml_get(__stnz, "body", -1);

  // if no body
  // TODO Filter <event> and <data> messages
  if (!_stnz)
    {
      TRACE("No Body\n");
      return;
    }

  if (_stnz->txt)
    TRACE("Message = '%s'\n", _stnz->txt);

  if (!c_pid)
    if (create_shell_job(sess, ezxml_attr(__stnz, "from")))
      return;

  // handle special characters
  if (!strcmp("^c", _stnz->txt))
    {
      if (_stnz->txt)
        TRACE("SPECIAL CHARACTER = '%s'\n", _stnz->txt);
      //                c = 0x3;
      //                write(shell_in[1],&c,1);
      err = write(shell_in[1], "$'\x3'\n", strlen("$'\x3'\n"));
    }
  else if (!strncmp("#send ", _stnz->txt, strlen("#send ")))
    {
#if 0
      /* FIXME file sending test (REMOVE THIS!!!) */
      xmpp_send_file(sess, str, _stnz->txt + strlen("#send "));
#endif
    }
  else
    {
      err = write(shell_in[1], _stnz->txt, strlen(_stnz->txt));
      // end line
      err = write(shell_in[1], "\n", strlen("\n"));
    }
}

/**
 * On iq handler
 * TODO It's not optimized handler!!
 * Spaguetti logic...
 */
static void
my_on_iq(struct x_bus *sess, ezxml_t __stnz)
{

  ezxml_t _stnz;
  char *str = NULL;
  char *iq_id;

  ENTER;

  if (__stnz == NULL)
    return;

  if (!my_is_stanza_acceptable(sess, __stnz))
    return;

  str = (char *) ezxml_attr(__stnz, "type");
  iq_id = (char *) ezxml_attr(__stnz, "id");

  TRACE("->> IQ id='%s', type='%s'\n---\n '%s'\n---\n",
      iq_id,str,__stnz->txt);
  if (!strcmp("error", str))
    xmpp_stanza2stdout(__stnz->child->sibling);

  if (__stnz->child)
    {
      _stnz = __stnz->child;

      TRACE("->> IQ child='%s'\n",
          _stnz->name);

      str = (char *) ezxml_attr(_stnz, "xmlns");
      // route stanza
      dbm_notify_providers(str, sess, _stnz);
    }

}

struct xmpp_handler myhndlr =
  { .on_message = my_on_message, .on_iq = my_on_iq, };

