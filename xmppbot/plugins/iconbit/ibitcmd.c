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
 * Created on: Sep 20, 2010
 *     Author: Artur Emagulov
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <ezxml.h>
#include <xmppagent.h>
#include <xforms.h>

#define ICONBIT_MSG_SENDER_FNAME "/tmp/msg_sender.txt"
#define ICONBIT_UPLOAD_DIR "/tmp/watch/"

/**
 *
 */
static
ezxml_t ibit_on_config(struct x_bus *sess, ezxml_t form)
{
	char *func = NULL;
	char *action, *sessid;

	ezxml_t _stnz1 = NULL;
	ezxml_t _stnz2 = NULL;
	ezxml_t _stnz3 = NULL;
	ezxml_t _stnz4 = NULL;
	ezxml_t _xform = NULL;

	func = (char *)ezxml_attr(form,XML_TAG_NODE);
	action = (char *)ezxml_attr(form,XML_TAG_ACTION);
	sessid = (char *)ezxml_attr(form,XML_TAG_SESSIONID);

	// create command response
	_stnz1 = ezxml_new(XML_TAG_COMMAND);
	ezxml_set_attr(_stnz1,XML_TAG_XMLNS,XMLNS_COMMANDS);
	ezxml_set_attr_d(_stnz1,XML_TAG_NODE,func);
	if (sessid != NULL)
		ezxml_set_attr_d(_stnz1,XML_TAG_SESSIONID,sessid);

	_xform = ezxml_get(form,XML_TAG_X,-1);

	/* enter critical section */
	pthread_mutex_lock(&sess->lock);

	// switch xforms command
	if ((action != NULL && !strcmp(action,XML_TAG_CANCEL)) || (_xform != NULL
			&& !strcmp(xforms_get_type(_xform), XML_TAG_CANCEL)))
	{
		// TODO Change status upon parameters list!
		ezxml_set_attr(_stnz1, XML_TAG_STATUS, "canceled");
		_stnz2 = ezxml_add_child(_stnz1, XML_TAG_NOTE, 0);
		ezxml_set_attr(_stnz2, XML_TAG_TYPE, "info");
		ezxml_set_txt(_stnz2, "Command canceled");
	}
	// return command help
	else if (!_xform ||
			strcmp(xforms_get_type(_xform),XML_TAG_SUBMIT))
	{
		// TODO Change status upon parameters list!
		ezxml_set_attr(_stnz1,XML_TAG_STATUS,"executing");
		// add actions node
		_stnz2 = ezxml_add_child(_stnz1, "actions",0);
		ezxml_set_attr(_stnz2,"execute","complete");
		_stnz2 = ezxml_add_child(_stnz2, "complete",0);

		// add xforms
		_stnz2 = ezxml_add_child(_stnz1, XML_TAG_X,0);
		ezxml_set_attr(_stnz2,XML_TAG_XMLNS,"jabber:x:data");
		ezxml_set_attr(_stnz2,XML_TAG_TYPE,"form");

		/* file transfer proxy */
		_stnz3 = ezxml_add_child(_stnz2, XML_TAG_FIELD,0);
		ezxml_set_attr(_stnz3,"var","ftproxy");
		ezxml_set_attr(_stnz3,XML_TAG_TYPE,"text-single");
		ezxml_set_attr(_stnz3,"label","File Transfer proxy hostname");
		_stnz3 = ezxml_add_child(_stnz3, "value",0);

		_stnz4 = ezxml_child(sess->dbcore,"ftproxy");
		if (_stnz4 != NULL)
			ezxml_set_txt(_stnz3,ezxml_attr(_stnz4,"jid"));

		_stnz3 = ezxml_add_child(_stnz2, XML_TAG_FIELD,0);
		ezxml_set_attr(_stnz3,"var","stunsrvname");
		ezxml_set_attr(_stnz3,XML_TAG_TYPE,"text-single");
		ezxml_set_attr(_stnz3,"label","STUN server hostname");
		_stnz3 = ezxml_add_child(_stnz3, "value",0);
		_stnz4 = ezxml_child(sess->dbcore,"stunserver");
		if (_stnz4 != NULL)
			ezxml_set_txt(_stnz3,ezxml_attr(_stnz4,"jid"));

		_stnz3 = ezxml_add_child(_stnz2,XML_TAG_FIELD,0);
		ezxml_set_attr(_stnz3,"var","stunsrvport");
		ezxml_set_attr(_stnz3,XML_TAG_TYPE,"text-single");
		ezxml_set_attr(_stnz3,"label","STUN port");
		_stnz3 = ezxml_add_child(_stnz3, "value",0);
		_stnz4 = ezxml_child(sess->dbcore,"stunserver");
		if (_stnz4 != NULL)
			ezxml_set_txt(_stnz3,ezxml_attr(_stnz4,"port"));

	}
	else /*submit*/
	{
		_stnz2 = ezxml_child(form,XML_TAG_X);
		for (_stnz2 = ezxml_child(_stnz2,XML_TAG_FIELD); _stnz2; _stnz2 = ezxml_next(_stnz2))
		{
			if (!strcmp("ftproxy", ezxml_attr(_stnz2,"var")))
			{
				_stnz3 = ezxml_child(_stnz2,"value");
				if (_stnz3)
				{
					_stnz4 = ezxml_child(sess->dbcore,"ftproxy");
					if (!_stnz4)
						_stnz4 = ezxml_add_child(sess->dbcore,"ftproxy",0);
					ezxml_set_attr_d(_stnz4,"jid",ezxml_txt(_stnz3));
				}
			}
			else if (!strcmp("stunsrvname", ezxml_attr(_stnz2,"var")))
			{
				_stnz3 = ezxml_child(_stnz2,"value");
				if (_stnz3)
				{
					_stnz4 = ezxml_child(sess->dbcore,"stunserver");
					if (!_stnz4)
						_stnz4 = ezxml_add_child(sess->dbcore,"stunserver",0);
					ezxml_set_attr_d(_stnz4,"jid",ezxml_txt(_stnz3));
				}
			}
			else if (!strcmp("stunsrvport", ezxml_attr(_stnz2,"var")))
			{
				_stnz3 = ezxml_child(_stnz2,"value");
				if (_stnz3)
				{
					_stnz4 = ezxml_child(sess->dbcore,"stunserver");
					if (!_stnz4)
						_stnz4 = ezxml_add_child(sess->dbcore,"stunserver",0);
					ezxml_set_attr_d(_stnz4,"port",ezxml_txt(_stnz3));
				}
			}
		}

		/* reply form */
		ezxml_set_attr(_stnz1,"status","completed");
		_stnz2 = ezxml_add_child(_stnz1, "note",0);
		ezxml_set_attr(_stnz2,"type","info");
		ezxml_set_txt(_stnz2, "Command completed successfully");

		// add xforms
		_stnz2 = ezxml_add_child(_stnz1, "x",0);
		ezxml_set_attr(_stnz2,XML_TAG_XMLNS,"jabber:x:data");
		ezxml_set_attr(_stnz2,XML_TAG_TYPE,"result");

		// First field (File Transfer Proxy)
		_stnz3 = ezxml_add_child(_stnz2, "field",0);
		ezxml_set_attr(_stnz3,XML_TAG_TYPE,"fixed");
		_stnz3 = ezxml_add_child(_stnz3, "value",0);
		ezxml_set_txt(_stnz3,"Configuration completed!");

	}

	xmpp_stanza2stdout(sess->dbcore);

	/* leave critical section */
	pthread_mutex_unlock(&sess->lock);

	return _stnz1;
}


/**
 *
 */
static
ezxml_t ibit_on_ibitrawctl(struct x_bus *sess, ezxml_t form)
{
	char *func = NULL;
	char *action,*sessid;
	char *ctlstr;
	int fd;
	ezxml_t _stnz1 = NULL;
	ezxml_t _stnz2 = NULL;
	ezxml_t _xform = NULL;

	func = (char *)ezxml_attr(form,XML_TAG_NODE);
	action = (char *)ezxml_attr(form,XML_TAG_ACTION);
	sessid = (char *)ezxml_attr(form,XML_TAG_SESSIONID);

	// create command response
	_stnz1 = ezxml_new("command");
	ezxml_set_attr(_stnz1,XML_TAG_XMLNS,XMLNS_COMMANDS);
	ezxml_set_attr_d(_stnz1,XML_TAG_NODE,func);
	if (sessid != NULL)
		ezxml_set_attr_d(_stnz1,"sessionid",sessid);

	_xform = ezxml_get(form, "x", -1);

	// switch xforms command
	if ((action != NULL && !strcmp(action,"cancel")) || (_xform != NULL
			&& !strcmp(xforms_get_type(_xform), "cancel")))
	{
		// TODO Change status upon parameters list!
		ezxml_set_attr(_stnz1, "status", "canceled");
		_stnz2 = ezxml_add_child(_stnz1, "note", 0);
		ezxml_set_attr(_stnz2, XML_TAG_TYPE, "info");
		ezxml_set_txt(_stnz2, "Command canceled");
	}
	else
	{

		ctlstr = xforms_get_value_of(_xform, "rawcmd");
		if (ctlstr)
		{
			printf("EXECUTING = '%s'\n", ctlstr);
			fd = open(ICONBIT_MSG_SENDER_FNAME, O_APPEND | O_RDWR | O_CREAT,
					S_IRWXU);
			if (fd > 0) {
				write(fd, ctlstr, strlen(ctlstr));
				write(fd, "\n", 1);
				close(fd);
			}
		}
		// TODO Change status upon parameters list!
		ezxml_set_attr(_stnz1, "status", "executing");
		// add actions node
		_stnz2 = ezxml_add_child(_stnz1, "actions", 0);
		ezxml_set_attr(_stnz2, "execute", "next");
		_stnz2 = ezxml_add_child(_stnz2, "next", 0);

		// add xforms
		_stnz2 = ezxml_add_child(_stnz1, "x", 0);
		ezxml_set_attr(_stnz2, "xmlns", "jabber:x:data");
		ezxml_set_attr(_stnz2, "type", "form");

		_stnz2 = ezxml_add_child(_stnz2, "field", 0);
		ezxml_set_attr(_stnz2, "var", "rawcmd");
		ezxml_set_attr(_stnz2, "type", "text-single");

	}

	return _stnz1;
}


/**
 *
 */
static
ezxml_t ibit_on_restartcmd(struct x_bus *sess, ezxml_t form)
{
	char *func = NULL;
	char *action,*sessid;
	char *ctlstr;
	ezxml_t _stnz1 = NULL;
	ezxml_t _stnz2 = NULL;
	ezxml_t _stnz3 = NULL;
	ezxml_t _xform = NULL;

	func = (char *)ezxml_attr(form,XML_TAG_NODE);
	action = (char *)ezxml_attr(form,XML_TAG_ACTION);
	sessid = (char *)ezxml_attr(form,XML_TAG_SESSIONID);

	// create command response
	_stnz1 = ezxml_new("command");
	ezxml_set_attr(_stnz1,XML_TAG_XMLNS,XMLNS_COMMANDS);
	ezxml_set_attr_d(_stnz1,XML_TAG_NODE,func);
	if (sessid != NULL)
		ezxml_set_attr_d(_stnz1,"sessionid",sessid);

	_xform = ezxml_get(form, "x", -1);

	// switch xforms command
	if ((action != NULL && !strcmp(action,"cancel")) || (_xform != NULL
			&& !strcmp(xforms_get_type(_xform), "cancel")))
	{
		// TODO Change status upon parameters list!
		ezxml_set_attr(_stnz1, "status", "canceled");
		_stnz2 = ezxml_add_child(_stnz1, "note", 0);
		ezxml_set_attr(_stnz2, XML_TAG_TYPE, "info");
		ezxml_set_txt(_stnz2, "Command canceled");
	}
	else
	{

		ctlstr = xforms_get_value_of(_xform, "agree");
		if (ctlstr && !strcmp(ctlstr,"ok"))
		{
			exit(0);
		}

		// TODO Change status upon parameters list!
		ezxml_set_attr(_stnz1, "status", "executing");
		// add actions node
		_stnz2 = ezxml_add_child(_stnz1, "actions", 0);
		ezxml_set_attr(_stnz2, "execute", "cancel");
		_stnz3 = ezxml_add_child(_stnz2, "cancel", 0);
		_stnz3 = ezxml_add_child(_stnz2, "complete", 0);

		// add xforms
		_stnz2 = ezxml_add_child(_stnz1, "x", 0);
		ezxml_set_attr(_stnz2, XML_TAG_XMLNS, "jabber:x:data");
		ezxml_set_attr(_stnz2, XML_TAG_TYPE, "form");

		_stnz3 = ezxml_add_child(_stnz2, "field", 0);
		ezxml_set_attr(_stnz3, "var", "agree");
		ezxml_set_attr(_stnz3, "type", "hidden");
		_stnz3 = ezxml_add_child(_stnz3, "value",0);
		ezxml_set_txt(_stnz3,"ok");

		// First field (File Transfer Proxy)
		_stnz3 = ezxml_add_child(_stnz2, "field",0);
		ezxml_set_attr(_stnz3,XML_TAG_TYPE,"fixed");
		_stnz3 = ezxml_add_child(_stnz3, "value",0);
		ezxml_set_txt(_stnz3,"ARE YOU SURE?");

	}

	return _stnz1;
}


/**
 *
 */
static struct xmlns_provider_item cmditems[] =
{
		{.name = "restart",.desc = "XmppBot EXIT!", .on_item = ibit_on_restartcmd},
		{.name = "ibitrawctl",.desc = "XmppBot IconBit raw control", .on_item = ibit_on_ibitrawctl},
		{.name = "config",.desc = "XmppBot config", .on_item = ibit_on_config},
		{0,}
};

/**
 *
 */
static int ibit_get_cmd_id(char *cmd)
{
	int i;
	if (!cmd) return -1;
	for (i=0;cmditems[i].name;i++)
	{
		if (!strcmp(cmditems[i].name,cmd))
			return i;
	}
	return -1;
}

/**
 *
 */
static int
on_ibit_cmd_stanza(struct x_bus *sess, ezxml_t __stnz)
{
	char *func = NULL;
	int id;
	ezxml_t _stnz1 = NULL;

	ENTER;

	func = (char *)ezxml_attr(__stnz,"node");

	id = ibit_get_cmd_id(func);
	if (id >= 0 && cmditems[id].on_item)
	{
		_stnz1 = cmditems[id].on_item(sess,__stnz);
		/* create Error reply */
		if (!_stnz1)
		{
			_stnz1 = ezxml_new("error");
			ezxml_set_attr(_stnz1,"type","modify");
			ezxml_set_attr_d(_stnz1,"code","400");
		}
		xmpp_session_reply_iq_stanza(sess,__stnz,_stnz1);
	}

	EXIT;
	return 0;
}

static struct xmlns_provider cmd_provider =
{ .stanza_rx = on_ibit_cmd_stanza,
.items = cmditems};

static
void ibitcmd_init(void)
{
	dbm_add_xmlns_provider(XMLNS_COMMANDS, &cmd_provider);
	return;
}

PLUGIN_INIT(ibitcmd_init);
