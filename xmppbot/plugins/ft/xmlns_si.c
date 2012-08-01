/*
 * Copyright (c) 2010, Artur Emagulov
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
 * CXmppAgent
 * Created on: Sep 21, 2010
 *     Author: Artur Emagulov
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#include <ezxml.h>
#include <xmppagent.h>

#if 0
static void ___send_test_cmd(struct x_bus *sess, ezxml_t _from)
  {
    ezxml_t _s = NULL;
    ezxml_t __stnz = NULL;
    ezxml_t _stnz1 = NULL;
    ezxml_t _stnz2 = NULL;

    ENTER;

    __stnz = xmpp_stanza_get_root(_from);
    if (!__stnz)
    return;

    _stnz1 = ezxml_new("message");
    _s = _stnz1;
    ezxml_set_attr_d(_s,"to",ezxml_attr(__stnz,"from"));
    ezxml_set_attr(_s, "from", sess->jid);
    //	ezxml_set_attr(_stnz1,"xmlns","http://jabber.org/protocol/feature-neg");

    // add xforms
    _stnz2 = ezxml_add_child(_stnz1, "x",0);
    ezxml_set_attr(_stnz2,"xmlns","jabber:x:data");
    ezxml_set_attr(_stnz2,"type","form");

    _stnz2 = ezxml_add_child(_stnz2, "field",0);
    ezxml_set_attr(_stnz2,"var","fname");
    ezxml_set_attr(_stnz2,"type","text-single");
    ezxml_set_attr(_stnz2,"label","Select filename");

    // ezxml_insert(_stnz1, _s, 0);
    xmpp_session_send_stanza(sess, _s);
    ezxml_free(_s);

    EXIT;

    return;
  }
#endif

/**
 * \return <si/> response to <si/> request
 */
static int
xmlns_si_file_accept(struct x_bus *sess, ezxml_t __stnz)
{
  char *str;

  int err;
  ezxml_t stream_job;

  ezxml_t _stnz1;
  ezxml_t _stnz2;
  char *fname, *siz;

  ENTER;

  _stnz1 = ezxml_child(__stnz, "file");

  if (!_stnz1)
    goto ftexception;

  fname = (char *) ezxml_attr(_stnz1, "name");
  siz = (char *) ezxml_attr(_stnz1, "size");

  // get features
  for (_stnz2 = ezxml_get(__stnz, "feature", -1); _stnz2; _stnz2
      = ezxml_next(_stnz2))
    {
      str = (char *) ezxml_attr(_stnz2, "xmlns");
      if (str && strstr(str, "feature-neg"))
        break;
    };

  if (!_stnz2)
    goto ftexception;

  // find stream-method
  for (_stnz2 = ezxml_get(_stnz2, "x", 0, "field", -1); _stnz2; _stnz2
      = ezxml_next(_stnz2))
    {
      str = (char *) ezxml_attr(_stnz2, "var");
      if (str && strstr(str, "stream-method"))
        break;
    };

  if (!_stnz2)
    goto ftexception;

  // get all stream methods
  for (_stnz2 = ezxml_child(_stnz2, "option"); _stnz2; _stnz2
      = ezxml_next(_stnz2))
    {
      _stnz1 = ezxml_child(_stnz2, "value");
      if (_stnz1)
        printf("STREAM: %s\n", _stnz1->txt);
      if (!strcmp(_stnz1->txt, XMLNS_BYTESTREAMS))
        break;
    };

  if (!_stnz2)
    goto ftexception;

  /* enter critical section */
  pthread_mutex_lock(&sess->lock);
  // create new file transfer job
  _stnz2 = ezxml_get(sess->dbcore, "bytestreams", 0, "pending", -1);
  if (!_stnz2)
    {
      _stnz2 = ezxml_child(sess->dbcore, "bytestreams");
      _stnz2 = ezxml_add_child(_stnz2, "pending", 0);
    }
  err = !_stnz2->child;
  /* if some pending job already exists */
  if (err)
    {
      stream_job = ezxml_new("job");
      ezxml_set_attr(stream_job, "type", "file-transfer");
      ezxml_set_attr_d(stream_job, "id", ezxml_attr(__stnz,"id"));
      if (ezxml_attr(__stnz, "mime-type"))
        ezxml_set_attr_d(stream_job, "mime-type", ezxml_attr(__stnz,"mime-type"));
      ezxml_set_attr_d(stream_job, "profile", ezxml_attr(__stnz,"profile"));
      ezxml_set_attr_d(stream_job, "name", fname);
      ezxml_set_attr(stream_job, "source", "remote");
      ezxml_set_attr(stream_job, "src_fd", "0");
      ezxml_set_attr(stream_job, "dst_fd", "0");
      ezxml_set_attr(stream_job, "status", "0");

      _stnz2 = ezxml_get(sess->dbcore, "bytestreams", 0, "pending", -1);
      ezxml_insert(stream_job, _stnz2, 0);

      /* leave critical section */
      pthread_mutex_unlock(&sess->lock);
    }
  else
    {
      /* leave critical section */
      pthread_mutex_unlock(&sess->lock);
      goto ftexception;
    }

  // create si response
  _stnz1 = ezxml_new("si");
  ezxml_set_attr(_stnz1, "xmlns", XMLNS_SI);

  _stnz2 = ezxml_add_child(_stnz1, "feature", 0);
  ezxml_set_attr(_stnz2, "xmlns", XMLNS_FEATURE_NEG);

  _stnz2 = ezxml_add_child(_stnz2, "x", 0);
  ezxml_set_attr(_stnz2, "xmlns", XMLNS_X_DATA);
  ezxml_set_attr(_stnz2, "type", "submit");

  _stnz2 = ezxml_add_child(_stnz2, "field", 0);
  ezxml_set_attr(_stnz2, "var", "stream-method");

  _stnz2 = ezxml_add_child(_stnz2, "value", 0);
  ezxml_set_txt(_stnz2, XMLNS_BYTESTREAMS);

  EXIT;

  xmpp_session_reply_iq_stanza(sess,__stnz,_stnz1);

  return 0;

  ftexception:
  // create si response
  _stnz1 = ezxml_new("error");
  ezxml_set_attr(_stnz1, "code", "400");
  ezxml_set_attr_d(_stnz1,"type","cancel");

  _stnz2 = ezxml_add_child(_stnz1, "bad-request", 0);
  ezxml_set_attr(_stnz2, "xmlns", XMLNS_XMPP_STANZAS);
  _stnz2 = ezxml_add_child(_stnz1, "no-valid-streams", 0);
  ezxml_set_attr(_stnz2, "xmlns", XMLNS_SI);

  EXIT;

  xmpp_session_reply_iq_stanza(sess,__stnz,_stnz1);

  return -1;

}

/**
 *
 */
static void *
sock5srv(void *arg)
{
  struct x_bus *sess;
  char buf[128];
  int sock;
  int sockcli;
  int err;
  int dst_fd;
  int port = 5277;
  ezxml_t _s;

  struct sockaddr_in srv;
  struct sockaddr_in cli;

  sess = (struct x_bus *) arg;

  _s = ezxml_get(sess->dbcore, "bytestreams", 0, "pending", -1);
  if (_s)
    _s = ezxml_child(_s, "job");

  ENTER;
  printf("Serving on %d\n", port);

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  memset(&srv, 0, sizeof(struct sockaddr_in));
  srv.sin_family = AF_INET;
  srv.sin_addr.s_addr = htonl(INADDR_ANY);
  srv.sin_port = htons(port);

  if (bind(sock, (struct sockaddr *) &srv, sizeof(struct sockaddr_in)) < 0)
    {
      perror("bind()");
      close(sock);
      return NULL;
    }

  if (listen(sock, 1) < 0)
    {
      perror("listen()");
      close(sock);
      return NULL;
    }

  printf("Accepting on %d\n", port);

  if ((sockcli = accept(sock, (struct sockaddr *) &cli, (socklen_t *) &err))
      < 0)
    {
      perror("accept()");
      close(sock);
    }

  err = recv(sockcli, buf, 128, 0);
#ifdef TRACE_DEBUG
  printf("Received %d bytes (0x%x 0x%x) from server\n", err, buf[0], buf[1]);
#endif

  buf[0] = 0x05;
  buf[1] = 0x00;

  err = send(sockcli, buf, 2, 0);
#ifdef TRACE_DEBUG
  printf("Sent %d bytes to server\n", err);
#endif

  err = recv(sockcli, buf, 128, 0);
#ifdef TRACE_DEBUG
  printf("Received %d bytes (0x%x 0x%x) from server\n", err, buf[0], buf[1]);
#endif

  buf[0] = 0x05;
  buf[1] = 0x00;

  err = send(sockcli, buf, err, 0);
#ifdef TRACE_DEBUG
  printf("Sent %d bytes to server\n", err);
#endif

  /*
   * Change jobs status and schedule
   */
  pthread_mutex_lock(&sess->lock);
  dst_fd = sockcli;
  sprintf(buf, "%d", dst_fd);
  ezxml_set_attr_d(_s,"dst_fd",buf);
  pthread_mutex_unlock(&sess->lock);

  close(sock);
  EXIT;
  return NULL;
}

/**
 *
 */
static void
xmlns_si_file_send(struct x_bus *sess, ezxml_t __stnz)
{
  ezxml_t _stnz;
  ezxml_t _s, _s1;
  int status;
  int has_proxy = 0;
  char *proxy = NULL;
  char *myjid = NULL;
  char *myip = NULL;

  ENTER;

  pthread_mutex_lock(&sess->lock);
  myjid = x_strdup(ezxml_attr(sess->dbcore, "jid"));
  _s = ezxml_get(sess->dbcore, "interfaces", 0, "iface", -1);
  myip = x_strdup(ezxml_attr(_s, "addr"));
  pthread_mutex_unlock(&sess->lock);

  _s = ezxml_get(sess->dbcore, "bytestreams", 0, "pending", -1);
  if (_s)
    _s = ezxml_child(_s, "job");

  _stnz = ezxml_get(sess->dbcore, "ftproxy", -1);
  proxy = (char *) ezxml_attr(_stnz, "jid");
  if (proxy)
    has_proxy++;

  sscanf(ezxml_attr(_s, "status"), "%d", &status);

  if (status < 2 && has_proxy)
    {
      // _s = xmpp_stanza_get_root(_from);
      // first send request to proxy
      _stnz = ezxml_new("iq");
      ezxml_set_attr(_stnz, "id", "6756756");
      ezxml_set_attr(_stnz, "type", "get");
      ezxml_set_attr(_stnz, "from", myjid);
      ezxml_set_attr_d(_stnz,"to",proxy);

      _s1 = ezxml_add_child(_stnz, "query", 0);
      ezxml_set_attr(_s1, "xmlns", XMLNS_BYTESTREAMS);

    }
  else if (myip)
    {
      _stnz = ezxml_new("iq");
      ezxml_set_attr(_stnz, "id", "6756756");
      ezxml_set_attr(_stnz, "type", "set");
      ezxml_set_attr(_stnz, "from", myjid);
      ezxml_set_attr_d(_stnz,"to",ezxml_attr(_s,"jid"));

      // FIXME Correct random values
      _s1 = ezxml_add_child(_stnz, "query", 0);
      ezxml_set_attr(_s1, "xmlns", XMLNS_BYTESTREAMS);
      ezxml_set_attr(_s1, "mode", "tcp");

      ezxml_set_attr_d(_s1,"sid",ezxml_attr(_s,"id"));

      // FIXME Correct ip
      _s1 = ezxml_add_child(_s1, "streamhost", 0);
      ezxml_set_attr(_s1, "jid", myjid);
      ezxml_set_attr_d(_s1,"host",myip);
      ezxml_set_attr(_s1, "port", "5277");
    }
  else
    {
      /* TODO Clean file atrsnfer job */
      // ERROR
    }

  EXIT;
  xmpp_session_send_stanza(sess, _stnz);
  ezxml_free(_stnz);
  if (myjid)
    free(myjid);
  if (myip)
    free(myip);
  return;
}

/**
 *
 */
static int
on_si_stanza(struct x_bus *sess, ezxml_t __stnz)
{
  ezxml_t s1;
  ENTER;

  // TODO This is invalid state processing: FIXME!!!!!!!
  // FIXME
  // FIXME
  s1 = ezxml_get(sess->dbcore, "bytestreams", 0, "pending", 0, "job", -1);
  if (!s1)
    xmlns_si_file_accept(sess, __stnz);
  else
    xmlns_si_file_send(sess, __stnz);
  EXIT;
  return 0;
}

/**
 *
 */
static int
xmpp_send_file(struct x_bus *sess, const char *jid, const char *name)
{
  struct stat f_stat;
  char ts[25];
  pthread_t t;
  ezxml_t stream_job;
  ezxml_t __stnz;
  ezxml_t _s1;
  ezxml_t _s2;
  char *myjid = NULL;

  if (stat(name, &f_stat))
    return -1;

  pthread_mutex_lock(&sess->lock);
  myjid = x_strdup(ezxml_attr(sess->dbcore, "jid"));
  pthread_mutex_unlock(&sess->lock);

  __stnz = ezxml_new("iq");
  ezxml_set_attr(__stnz, "type", "set");
  ezxml_set_attr_d(__stnz,"to",jid);
  ezxml_set_attr(__stnz, "from", myjid);
  // TODO Randomize ID
  ezxml_set_attr(__stnz, "id", "some-id");

  _s1 = ezxml_add_child(__stnz, "si", 0);
  ezxml_set_attr(_s1, "xmlns", XMLNS_SI);
  sprintf(ts, "s5b_%x", (int) (random() + time(NULL)));
  ezxml_set_attr_d(_s1,"id",ts);
  ezxml_set_attr(_s1, "profile", XMLNS_FILE_TRANSFER);

  _s2 = ezxml_add_child(_s1, "file", 0);
  ezxml_set_attr(_s2, "xmlns", XMLNS_FILE_TRANSFER);
  ezxml_set_attr_d(_s2,"name",name);
  sprintf(ts, "%d", (int) f_stat.st_size);
  ezxml_set_attr_d(_s2,"size",ts);

  _s2 = ezxml_add_child(_s1, "feature", 0);
  ezxml_set_attr(_s2, "xmlns", XMLNS_FEATURE_NEG);

  _s2 = ezxml_add_child(_s2, "x", 0);
  ezxml_set_attr(_s2, "xmlns", XMLNS_X_DATA);
  ezxml_set_attr(_s2, "type", "form");

  _s2 = ezxml_add_child(_s2, "field", 0);
  ezxml_set_attr(_s2, "var", "stream-method");
  ezxml_set_attr(_s2, "type", "list-single");

  _s2 = ezxml_add_child(_s2, "option", 0);
  _s2 = ezxml_add_child(_s2, "value", 0);
  ezxml_set_txt(_s2, XMLNS_BYTESTREAMS);

  /* enter critical section */
  pthread_mutex_lock(&sess->lock);
  // create new file transfer job
  // this job should be created and added to
  // new main event loop
  _s2 = ezxml_get(sess->dbcore, "bytestreams", 0, "pending", -1);
  if (!_s2)
    {
      _s2 = ezxml_get(sess->dbcore, "bytestreams", -1);
      _s2 = ezxml_add_child(_s2, "pending", 0);
    }
  _s2 = ezxml_child(_s2, "job");
  /* if some pending job already exists */
  if (!_s2)
    {
      // create new stream job
      stream_job = ezxml_new("job");
      ezxml_set_attr_d(stream_job, "jid", jid);
      ezxml_set_attr_d(stream_job, "id",ezxml_attr(_s1,"id"));
      ezxml_set_attr(stream_job, "type", "file-transfer");
      ezxml_set_attr(stream_job, "source", "local");
      ezxml_set_attr_d(stream_job, "name", name);
      ezxml_set_attr(stream_job, "src_fd", "0");
      ezxml_set_attr(stream_job, "dst_fd", "0");
      ezxml_set_attr(stream_job, "status", "0");

      _s2 = ezxml_get(sess->dbcore, "bytestreams", 0, "pending", -1);
      ezxml_insert(stream_job, _s2, 0);
      xmpp_stanza2stdout(sess->dbcore);
    }
  pthread_mutex_unlock(&sess->lock);

  // FIXME
  pthread_create(&t, NULL, sock5srv, (void *) sess);
  sprintf(ts, "%d", (int) t);
  pthread_mutex_lock(&sess->lock);
  _s2 = ezxml_get(sess->dbcore, "threads", -1);
  if (_s2)
    {
      _s2 = ezxml_add_child(_s2, "thread", 0);
      ezxml_set_attr(_s2, "name", "socks5");
      ezxml_set_attr_d(_s2,"id",ts);
    }
  pthread_mutex_unlock(&sess->lock);

  EXIT;

  xmpp_session_send_stanza(sess, __stnz);
  ezxml_free(__stnz);
  if (myjid)
    free(myjid);

  return 0;
}

static struct xmlns_provider si_provider =
  { .stanza_rx = on_si_stanza, };

void
si_proto_init(void)
{
  dbm_add_xmlns_provider(XMLNS_SI, &si_provider);
  return;
}
PLUGIN_INIT(si_proto_init)
;

