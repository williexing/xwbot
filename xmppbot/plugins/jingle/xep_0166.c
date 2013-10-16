/*
 * xep_0166.c
 *
 *  Created on: Jan 5, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX

#include <x_config.h>
#if TRACE_XJINGLE_ON
#define DEBUG_PRFX "[jingle] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>

#include <plugins/sessions_api/sessions.h>

#define JINGLE_NS_1 "urn:xmpp:jingle:1"

enum
{
    JINGLE_STATE_GNEW = 0, JINGLE_STATE_INCOMING, JINGLE_STATE_OUTCOMING,
};

enum
{
    JINGLE_CR0_PENDING = 0x1,
    JINGLE_CR0_RUNNING = 0x2,
    JINGLE_CR0_STOPPED = 0x4,
    JINGLE_CR0_REMOTE_CANDIDATE = 0x8
};

typedef struct jingle_object
{
    x_object super;
    x_object *iosession;
    x_obj_attr_t pending_channels;
    struct
    {
        int state;
        u_int32_t cr0;
    } regs;
} x_jingle_object_t;

__inline static void
_send_set_req_(x_object *to, x_object *msg, x_obj_attr_t *hints)
{
    char id[24];
    setattr("#type", "set", hints);
    x_snprintf(id, sizeof(id) - 1, "jingle%d", random());
    setattr("#id", id, hints);
    x_object_send_down(to, msg, hints);

//    attr_list_clear(hints);
}

static void
jingle_on_create(x_object *o)
{
    x_jingle_object_t *xjo = (x_jingle_object_t *) (void *) o;
    ENTER;
    xjo->pending_channels.next = 0;
    xjo->pending_channels.key = 0;
    xjo->pending_channels.val = 0;
    EXIT;
}

/* matching function */
static BOOL
jingle_match(x_object *o, x_obj_attr_t *_o)
{
    const char *tmpstr;
    const char *tmpstr2;
    ENTER;

    tmpstr = getattr("xmlns", _o);
    if (tmpstr && NEQ(tmpstr, "urn:xmpp:jingle:1"))
        return FALSE;

    tmpstr = getattr("sid", _o);
    tmpstr2 = x_object_getattr(o, "sid");

    if (!(tmpstr || tmpstr2) || ((tmpstr && tmpstr2) && EQ(tmpstr, tmpstr2)))
        return TRUE;

    return FALSE;

}


static void
__jingle_session_send_transport_info(x_object *o)
{
    x_obj_attr_t *attr;

    x_object *tmp;
    x_object *descr, *_descr_;
    x_object *transport;
    x_object *cand;
    x_object *_chan_;
    x_object *_tr_;
    x_obj_attr_t hints =
    { NULL, NULL, NULL, };
    x_object *msg;

    x_object *_sess_;
    x_object *cho;
    x_object *transp;
    x_object *pload;
    x_object *pparm;

    TRACE("\n");
    msg = _CPY(o);
    _ASET(msg, "action", "transport-info");
    _ASET(msg, "senders", NULL); // reset senders attr
    _ASET(msg, "xmlns", "urn:xmpp:jingle:1"); // TODO! this should not be hardcoded
    TRACE("\n");

    setattr("sid", (VAL) _ENV(o, "sid"), &hints);
    _sess_ = x_session_open(_ENV(o, "from"), o, &hints, 0);

    TRACE("\n");

    if (!_sess_)
    {
        TRACE("Invalid state: session not found sid=%s\n", _ENV(o, "sid"));
        return;
    }

    //  x_object_print_path(o,0);
    //  BUG();

    TRACE("\n");

    /**
   * Here we will accept jingle's part of session object,
   * this is not 'session' class, but 'jingle' class
   */
    for (cho = _CHLD(_sess_,NULL); cho; cho = _SBL(cho))
    {
        transp = _CHLD(cho, "transport");
        if (!transp)
            continue;

        TRACE("\n");

        _chan_ = _GNEW(_XS("$message"),NULL);
        _SETNM(_chan_,_XS("content"));
        _ASET(_chan_, "name", _GETNM(cho));
        /**
         * @bug a.a.e This params should be stored in __isession object
         */
        _ASET(_chan_, "creator", "initiator");
        _ASET(_chan_, "senders", "both");

        x_object_append_child_no_cb(msg, _chan_);


        _tr_ = _CPY(_CHLD(cho, "transport"));

        _ASET(_tr_, _XS("ufrag"), _AGET(transp,_XS("ufrag")));
        _ASET(_tr_, _XS("pwd"), _AGET(transp,_XS("pwd")));
        x_object_append_child_no_cb(_chan_, _tr_);


        // copy candidates if exists
        for (tmp = _CHLD(transp, _XS("local-candidate")); tmp; tmp = _NXT(tmp))
        {
            TRACE("\n");
            cand = _CPY(tmp);
            _SETNM(cand, _XS("candidate"));
            x_object_append_child_no_cb(_tr_, cand);
        }

#if 0
        TRACE("\n");
        descr = _CHLD(chan, "description");
        TRACE("\n");
        _descr_ = _CPY(descr);
        TRACE("\n");
        x_object_append_child_no_cb(_chan_, _descr_);
        TRACE("\n");

        // copy description
        for (tmp = _CHLD(descr, _XS("payload-type")); tmp; tmp = _NXT(tmp))
        {
            TRACE("\n");
            //          x_object_append_child_no_cb(_descr_, x_object_full_copy(tmp));
            pload = _GNEW(_XS("$message"),NULL);
            _SETNM(pload, _XS("payload-type"));
            //          iterate attrs

            for (attr = tmp->attrs.next; attr && attr != &tmp->attrs;
                 attr = attr->next)
            {
                TRACE("\n");
                if(attr->key && *attr->key == '$')
                    continue;

                if (EQ(attr->key,_XS("id")) || EQ(attr->key,_XS("name"))
                        || EQ(attr->key,_XS("clockrate")))
                {
                    _ASET(pload, attr->key, attr->val);
                }
                else
                {
                    pparm = _GNEW(_XS("$message"),NULL);
                    _SETNM(pparm, _XS("parameter"));
                    _ASET(pparm, _XS("name"), attr->key);
                    _ASET(pparm, _XS("value"), attr->val);
                    x_object_append_child_no_cb(pload, pparm);
                }
            }

            TRACE("\n");
            x_object_append_child_no_cb(_descr_, pload);
            TRACE("\n");

        }
#endif
    }

    TRACE("Transport info...\n");
    _send_set_req_(_PARNT(o), msg, &hints);
}


static void
__jingle_session_send_accept(x_object *o)
{
    x_obj_attr_t *attr;

    x_object *chan, *tmp;
    x_object *descr, *_descr_;
    x_object *transport;
    x_object *cand;
    x_object *_chan_;
    x_object *_tr_;
    x_obj_attr_t hints =
    { NULL, NULL, NULL, };
    x_object *msg;

    x_object *_sess_;
    x_object *cho;
    x_object *transp;
    x_object *pload;
    x_object *pparm;

    TRACE("\n");
    msg = _CPY(o);
    _ASET(msg, "action", "session-accept");
    TRACE("\n");

    setattr("sid", (VAL) _ENV(o, "sid"), &hints);
    _sess_ = x_session_open(_ENV(o, "from"), o, &hints, 0);

    TRACE("\n");

    if (!_sess_)
    {
        TRACE("Invalid state: session not found sid=%s\n", _ENV(o, "sid"));
        return;
    }

    //  x_object_print_path(o,0);
    //  BUG();

    TRACE("\n");

    /**
     * Here we will accept jingle's part of session object,
     * this is not 'session' class, but 'jingle' class
     */
    for (chan = _CHLD(o, "content"); chan; chan = _NXT(chan))
    {
        TRACE("\n");
        cho = x_session_channel_open2(_sess_, _AGET(chan,_XS("name")));
        TRACE("\n");
        if (!cho)
            continue;
        transp = _CHLD(cho, "transport");
        if (!transp)
            continue;
        TRACE("\n");

        TRACE("\n");
        _chan_ = _CPY(chan);
        TRACE("\n");
        x_object_append_child_no_cb(msg, _chan_);
        TRACE("\n");

        _tr_ = _CPY(_CHLD(chan, "transport"));
        TRACE("\n");
        _ASET(_tr_, _XS("ufrag"), _AGET(transp,_XS("ufrag")));
        _ASET(_tr_, _XS("pwd"), _AGET(transp,_XS("pwd")));
        x_object_append_child_no_cb(_chan_, _tr_);
        TRACE("\n");

        // copy candidates if exists
#if 0
        for (tmp = _CHLD(transp, _XS("local-candidate")); tmp; tmp = _NXT(tmp))
        {
            TRACE("\n");
            cand = _CPY(tmp);
            _SETNM(cand, _XS("candidate"));
            x_object_append_child_no_cb(_tr_, cand);
        }
#endif

#if 0
        TRACE("\n");
        descr = _CHLD(chan, "description");
        TRACE("\n");
        _descr_ = _CPY(descr);
        TRACE("\n");
        x_object_append_child_no_cb(_chan_, _descr_);
        TRACE("\n");

        // copy description
        for (tmp = _CHLD(descr, _XS("payload-type")); tmp; tmp = _NXT(tmp))
        {
            TRACE("\n");
            //          x_object_append_child_no_cb(_descr_, x_object_full_copy(tmp));
            pload = _GNEW(_XS("$message"),NULL);
            _SETNM(pload, _XS("payload-type"));
            //          iterate attrs

            for (attr = tmp->attrs.next; attr && attr != &tmp->attrs;
                 attr = attr->next)
            {
                TRACE("\n");
                if(attr->key && *attr->key == '$')
                    continue;

                if (EQ(attr->key,_XS("id")) || EQ(attr->key,_XS("name"))
                        || EQ(attr->key,_XS("clockrate")))
                {
                    _ASET(pload, attr->key, attr->val);
                }
                else
                {
                    pparm = _GNEW(_XS("$message"),NULL);
                    _SETNM(pparm, _XS("parameter"));
                    _ASET(pparm, _XS("name"), attr->key);
                    _ASET(pparm, _XS("value"), attr->val);
                    x_object_append_child_no_cb(pload, pparm);
                }
            }

            TRACE("\n");
            x_object_append_child_no_cb(_descr_, pload);
            TRACE("\n");

        }
#else
        /*---------- media --------*/
        descr = _GNEW("$message",NULL);
        _SETNM(descr,_XS("description"));

        _ASET(descr, _XS("xmlns"), _XS("urn:xmpp:jingle:apps:rtp:1"));

        _ASET(descr, _XS("media"), _AGET(cho,_XS("mtype")));

        x_object_append_child_no_cb(_chan_, descr);

        // get out$media entry (FIXME for one way channel)
        _descr_ = _CHLD(cho, MEDIA_OUT_CLASS_STR);
        if (!_descr_)
            continue;

        // copy description
        for (tmp = _CHLD(_descr_, NULL); tmp; tmp = _SBL(tmp))
        {
            //          x_object_append_child_no_cb(_descr_, x_object_full_copy(tmp));
            pload = _GNEW(_XS("$message"),NULL);
            _SETNM(pload, _XS("payload-type"));
            //          iterate attrs

            TRACE("NEW PAYLOAD\n");

            for (attr = tmp->attrs.next; attr != NULL;
                 attr = (x_obj_attr_t *)(void *)attr->next)
            {
                TRACE("%s=%s, next(%p)\n",
                      attr->key, attr->val,(void *)attr->next);
                if (EQ(attr->key,_XS("id")) || EQ(attr->key,_XS("name"))
                        || EQ(attr->key,_XS("clockrate")))
                {
                    _ASET(pload, attr->key, attr->val);
                }
                else
                {
                    if (attr->key && *attr->key != '$')
                    {
                        pparm = _GNEW(_XS("$message"),NULL);
                        _SETNM(pparm, _XS("parameter"));
                        _ASET(pparm, _XS("name"), attr->key);
                        _ASET(pparm, _XS("value"), attr->val);
                        x_object_append_child_no_cb(pload, pparm);
                    }
                }
            }

            x_object_append_child_no_cb(descr, pload);

        }
        /*------- END of construct channels entries */
#endif
    }

    TRACE("Accepting...\n");
    _send_set_req_(_PARNT(o), msg, &hints);
}

static void
__jingle_session_send_terminate(x_object *o)
{
    x_obj_attr_t hints =
    { NULL, NULL, NULL, };
    x_object *msg;
    x_object *reason;
    x_object *reason_gone;
//    const char *sid;

    ENTER;

    TRACE("Terminating sessions %s...\n",_ENV(o,"sid"));

    msg = _GNEW("$message",NULL);
    _SETNM(msg,"jingle");
    _ASET(msg, "action", "session-terminate");
    _ASET(msg, "xmlns", "urn:xmpp:jingle:1");
    _ASET(msg, "initiator", NULL);
    _ASET(msg, "sid", _ENV(o,"sid"));

    reason = _GNEW(_XS("$message"),NULL);
    reason_gone = _GNEW(_XS("$message"),NULL);

    _SETNM(reason,"reason");
    _SETNM(reason_gone,"gone");

    _INS(reason, reason_gone);
    _INS(msg, reason);

    x_object_print_path(o,0);
    x_object_print_path(msg,0);
    TRACE("Sending from '%s' to '%s'\n", _ENV(o,"to"),_ENV(o,"from"));

    _send_set_req_(_PARNT(o), msg, &hints);

    send_err:
    EXIT;

    TRACE("Ok...\n");

    return;
}

static void
__jingle_session_send_initiate(x_object *o)
{
    volatile x_obj_attr_t *attr;

    x_object *chan, *tmp;
    x_object *descr, *_descr_;
    x_object *transport;
    x_object *cand;
    x_object *_chan_;
    x_object *_tr_;
    x_obj_attr_t hints =
    { NULL, NULL, NULL, };
    x_object *msg;
    struct jingle_object *jnglo = (struct jingle_object *) (void *) o;

    x_object *_sess_;
    x_object *cho;
    x_object *transp;

    x_object *pload;
    x_object *pparm;

    msg = _CPY(o);
    _ASET(msg, "action", "session-initiate");
    _ASET(msg, "xmlns", "urn:xmpp:jingle:1");
    _ASET(msg, "initiator", _ENV(o,"jid"));

    setattr("sid", (VAL) _ENV(o, "sid"), &hints);
    _sess_ = x_session_open(_ENV(o, "from"), o, &hints, 0);

    if (!_sess_)
    {
        TRACE("Invalid state: session not found sid=%s\n", _ENV(o, "sid"));
        return;
    }

    //    x_object_print_path(_sess_, 0);

    for (chan = _CHLD(_sess_,NULL); chan; chan = _SBL(chan))
    {

        //        x_object_print_path(chan,0);

        /*-----------------------------------*/
        /*------- construct channels entries */
        transp = _CHLD(chan, _XS("transport"));
        if (!transp)
            continue;

        _chan_ = _GNEW("$message",NULL);
        _SETNM(_chan_,_XS("content"));

        _ASET(_chan_, _XS("name"), _GETNM(chan));
        /** @todo Theese two params should be retreived
       * from session channel */
        _ASET(_chan_, _XS("senders"), _XS("both"));
        _ASET(_chan_, _XS("creator"), _XS("initiator"));
        x_object_append_child_no_cb(msg, _chan_);

        /*---------- transport --------*/
        _tr_ = _CPY(transp);
        _ASET(_tr_, _XS("ufrag"), _AGET(transp,_XS("ufrag")));
        _ASET(_tr_, _XS("pwd"), _AGET(transp,_XS("pwd")));
        _ASET(_tr_, _XS("xmlns"), _AGET(transp,_XS("xmlns")));
        x_object_append_child_no_cb(_chan_, _tr_);
#if 0
        // copy candidates if exists
        for (tmp = _CHLD(transp, _XS("local-candidate")); tmp; tmp = _NXT(tmp))
        {
            cand = _CPY(tmp);
            _SETNM(cand, _XS("candidate"));
            x_object_append_child_no_cb(_tr_, cand);
        }
#endif
        /*---------- END of transport --------*/

        /*---------- media --------*/
        descr = _GNEW("$message",NULL);
        _SETNM(descr,_XS("description"));

        _ASET(descr, _XS("xmlns"), _XS("urn:xmpp:jingle:apps:rtp:1"));

        _ASET(descr, _XS("media"), _AGET(chan,_XS("mtype")));

        x_object_append_child_no_cb(_chan_, descr);

        // get out$media entry (FIXME for one way channel)
        _descr_ = _CHLD(chan, MEDIA_OUT_CLASS_STR);
        if (!_descr_)
            continue;

        // copy description
        for (tmp = _CHLD(_descr_, NULL); tmp; tmp = _SBL(tmp))
        {
            //          x_object_append_child_no_cb(_descr_, x_object_full_copy(tmp));
            pload = _GNEW(_XS("$message"),NULL);
            _SETNM(pload, _XS("payload-type"));
            //          iterate attrs

            TRACE("NEW PAYLOAD\n");

            for (attr = tmp->attrs.next; attr != NULL;
                 attr = (x_obj_attr_t *)(void *)attr->next)
            {
                TRACE("%s=%s, next(%p)\n",
                      attr->key, attr->val,(void *)attr->next);
                if (EQ(attr->key,_XS("id")) || EQ(attr->key,_XS("name"))
                        || EQ(attr->key,_XS("clockrate")))
                {
                    _ASET(pload, attr->key, attr->val);
                }
                else
                {
                    if (attr->key && *attr->key != '$')
                    {
                        pparm = _GNEW(_XS("$message"),NULL);
                        _SETNM(pparm, _XS("parameter"));
                        _ASET(pparm, _XS("name"), attr->key);
                        _ASET(pparm, _XS("value"), attr->val);
                        x_object_append_child_no_cb(pload, pparm);
                    }
                }
            }

            x_object_append_child_no_cb(descr, pload);

        }
        /*------- END of construct channels entries */

    }

    TRACE("Initiating...\n");
    // turn to pending state
    jnglo->regs.cr0 = JINGLE_CR0_PENDING;
    _send_set_req_(_PARNT(o), msg, &hints);
}

static void
__jingle_session_send_candidate(x_object *sesso, x_object *isess,
                                x_object *candmsg)
{
    x_obj_attr_t hints =
    { NULL, NULL, NULL, };
    x_object *msg;
    x_object *chan, *chan1;
    x_object *transport;
    x_object *cand, *cand1;
    const char *chname;

    ENTER;

    chname = _AGET(candmsg,_XS("channel-name"));
    if (!chname)
    {
        goto send_err;
    }

    chan1 = _CHLD(isess,chname);
    if (!chan1)
    {
        goto send_err;
    }

    cand1 = _CHLD(candmsg,_XS("candidate"));
    if (!cand1)
    {
        goto send_err;
    }

    chan = _CPY(chan1);
    _SETNM(chan, _XS("content"));
    _ASET(chan, _XS("name"), chname);
    _ASET(chan, _XS("creator"), _XS("initiator"));

    transport = _CPY(_CHLD(chan1,_XS("transport")));
    cand = _CPY(cand1);
    msg = _CPY(sesso);

    _ASET(msg, "action", "transport-info");

    _INS(transport, cand);
    _INS(chan, transport);
    _INS(msg, chan);

    _send_set_req_(_PARNT(sesso), msg, &hints);

    send_err:
        EXIT;

    return;
}

static void
__jingle_session_on_initiate(x_object *thiz_)
{
    int err;
    x_object *chan;
    x_object *descr;
    x_object *transport;
    x_object *_transport_;
    x_object *payload;
    x_object *_chan_;
    x_object *_sess_ = NULL;
    x_obj_attr_t hints =
    { NULL, NULL, NULL, };

    x_obj_attr_t params =
    { NULL, NULL, NULL, };

    const char *sid;
    const char *from;
    const char *tmpstr, *tmpstr2;
    struct jingle_object *thiz = (struct jingle_object *) (void *) thiz_;

    from = _ENV(thiz_, "from");
    BUG_ON(!from);

    if (from)
    {
        if (thiz->regs.state == JINGLE_STATE_GNEW
                || EQ(from, _ENV(thiz_,"jid"))
                )
        {
            sid = _AGET(thiz_, "sid");
            setattr("sid", (VAL) sid, &hints);
            TRACE("Opening session '%s'\n", sid);
            _sess_ = x_session_open(from, thiz_, &hints, X_CREAT);
            attr_list_clear(&hints);
        }
        else
        {
            TRACE("Jingle session already exists '%s'\n", sid);
            x_object_print_path(thiz_->bus,0);
            BUG();
        }
    }

    if (!_sess_)
    {
        BUG();
    }

    /**
   * Iterate over all pending channels
   */
    for (chan = _CHLD(thiz_, "content"); chan; chan = _NXT(chan))
    {
        _chan_ = x_session_channel_open2(_sess_, _AGET(chan,_XS("name")));
        if (!_chan_)
        {
            continue;
        }
        else
        {
            // set pending channel name
            setattr(_AGET(chan,"name"), _AGET(chan,"name"),
                    &thiz->pending_channels);
        }
    }

    //  x_object *rejected = _GNEW("#container",NULL);
    for (chan = _CHLD(thiz_, "content"); chan; chan = _NXT(chan))
    {
        _chan_ = x_session_channel_open2(_sess_, _AGET(chan,_XS("name")));
        if (!_chan_)
        {
            //          _INS(rejected, _CPY(chan));
            continue;
        }
        else
        {

            /**
           * Add transport profiles
           */
            transport = _CHLD(chan, "transport");
            tmpstr = _AGET(transport, "xmlns");
            //          tmpstr2= _AGET(transport, "xmlns:p");

            TRACE("TRANSPORT NAMESPACE: %s\n",tmpstr);

#warning "REMOVE THIS UGLY HACK"
#if 1
            if ((tmpstr && x_strstr(tmpstr, "ice")))
            {
                TRACE("Using icectl transport\n");
                x_session_channel_set_transport_profile_ns(
                            _chan_, _XS("__icectl"), _XS("urn:xmpp:jingle:transports:ice-udp:1"),
                            &transport->attrs);
            }
            else if ((tmpstr && x_strstr(tmpstr, "p2p")))
            {
                TRACE("Using icectl transport\n");
                x_session_channel_set_transport_profile_ns(
                            _chan_, _XS("__icectl"), "http://www.google.com/transport/p2p",
                            &transport->attrs);
            }
//            else
//            {
//                TRACE("Using STUB transport\n");
//                x_session_channel_set_transport_profile_ns(
//                            _chan_, _XS("__stub"),_XS("jingle"),
//                            &transport->attrs);
//            }
#else
            if(x_session_channel_set_transport_profile_ns(
                       _chan_, _XS("__icectl"), tmpstr,
                        &transport->attrs) < 0)
            {
                TRACE("ERROR!!\n");
                // TODO Should reply error
            }
#endif
            /* now assign params and append all candidates
              as is done in 'transport-info' handler
             */
            _transport_ = _CHLD(_chan_, "transport");
            if (_transport_)
            {
                x_object *candidate;
                x_string_t tmpstr1, tmpstr2;
                _ASGN(_transport_, (x_obj_attr_t *)&transport->attrs);

                for (candidate = _CHLD(transport, "candidate"); candidate; candidate =
                     _NXT(candidate))
                {
                    /* set username/password attributes for each candidate
               * for ICE/STUN object */
                    tmpstr1 = _ENV(candidate,"ufrag");
                    tmpstr2 = _ENV(candidate,"pwd");

                    TRACE("ADDING REMOTE CANDIDATE '%s:%s'\n", tmpstr1, tmpstr2);

                    _ASET(candidate, _XS("username"), tmpstr1);
                    _ASET(candidate, _XS("password"), tmpstr2);

                    x_session_channel_add_transport_candidate(_chan_, candidate);
                }
            }

            /**
           * Iterate over media profiles
           */
            descr = _CHLD(chan, "description");
            tmpstr = _AGET(descr, "xmlns");

            /* set media type information */
            attr_list_clear(&hints);
            setattr(_XS("mtype"), _AGET(descr,_XS("media")), &hints);
            _ASGN(_chan_, &hints);
            attr_list_clear(&hints);

            if (tmpstr && x_strstr(tmpstr, "rtp"))
            {

                x_session_channel_set_media_profile_ns(_chan_, _XS("__rtppldctl"),
                                                       _XS("jingle"));

                for (payload = _CHLD(descr, _XS("payload-type")); payload;
                     payload = _NXT(payload))
                {
                    x_object *tmp;
                    x_object *tmpparam;

                    const char *ptname;
                    ptname = _AGET(payload,_XS("name"));
                    TRACE("ADDING payload '%s'...\n", ptname);

                    // assign parameters
                    for (tmpparam = _CHLD(payload,_XS("parameter")); tmpparam;
                         tmpparam = _NXT(tmpparam))
                    {
                        setattr(_AGET(tmpparam,"name"), _AGET(tmpparam,"value"),
                                &payload->attrs);
                    }

                    /*
                     * Add both encoders and decoders here
                     * @todo this should be checked against 'senders' attribute of a channel
                     */
                    err = x_session_add_payload_to_channel(_chan_,
                                                           _AGET(payload,_XS("id")),
                                                           ptname, MEDIA_IO_MODE_IN, &payload->attrs);
                    if (err < 0) TRACE("Cannot set decoder media profile\n");

                    err = x_session_add_payload_to_channel(_chan_,
                                                           _AGET(payload,_XS("id")),
                                                           ptname, MEDIA_IO_MODE_OUT, &payload->attrs);
                    if (err < 0) TRACE("Cannot set encoder media profile\n");
                }
            }
            else
            {
                x_session_channel_set_media_profile_ns(_chan_, _XS("__stub"),
                                                       _XS("jingle"));
            }

            //            x_object_print_path(_chan_, 0);
            // commit channel
            setattr(_XS("$commit"), _XS("yes"), &hints);
            _ASGN(X_OBJECT(_chan_), &hints);
            attr_list_clear(&hints);

        }
    }

    // finally start session
    thiz->regs.cr0 = JINGLE_CR0_PENDING;

    x_session_start(_sess_);

    //  x_object_destroy_no_cb(rejected);
}

static void
__jingle_session_on_transport_info(x_object *o)
{
    x_object *chan;
    x_object *descr;
    x_object *transport;
    x_object *candidate;
    x_object *_chan_;
    x_object *_transport_;
    x_string_t tmpstr1, tmpstr2;

    struct jingle_object *thiz = (struct jingle_object *) (void *) o;

    ENTER;

    for (chan = _CHLD(o, "content"); chan; chan = _NXT(chan))
    {
        transport = _CHLD(chan, "transport");
        _chan_ = x_session_channel_open(o, _ENV(chan, "from"), _ENV(chan, "sid"),
                                        _AGET(chan, "name"));

        if (!_chan_)
            continue;

        _transport_ = _CHLD(_chan_, "transport");

        if (!_transport_)
            continue;

        _ASGN(_transport_, (x_obj_attr_t *)&transport->attrs);

        for (candidate = _CHLD(transport, "candidate"); candidate; candidate =
             _NXT(candidate))
        {
            /* set username/password attributes for each candidate
           * for ICE/STUN object */
            tmpstr1 = _ENV(candidate,"ufrag");
            tmpstr2 = _ENV(candidate,"pwd");

            TRACE("ADDING REMOTE CANDIDATE '%s(%s):%s'\n",
                  _AGET(candidate, _XS("ip")),
                  _AGET(candidate, _XS("address")),
                  _AGET(candidate, _XS("port")));

            if (tmpstr1 && !_AGET(candidate,"username"))
                _ASET(candidate, _XS("username"), tmpstr1);
            if (tmpstr2 && !_AGET(candidate,"password"))
                _ASET(candidate, _XS("password"), tmpstr2);

            x_session_channel_add_transport_candidate(_chan_, candidate);

            // fire flag
            thiz->regs.cr0 |= JINGLE_CR0_REMOTE_CANDIDATE;

        }
    }

    /* TODO! run jingle_state_step() */
    {
        if(thiz->regs.state == JINGLE_STATE_INCOMING
                && thiz->regs.cr0 & JINGLE_CR0_PENDING
                && thiz->regs.cr0 & JINGLE_CR0_REMOTE_CANDIDATE)
        {
            __jingle_session_send_accept(o);
            thiz->regs.cr0 &= ~JINGLE_CR0_PENDING;
            thiz->regs.cr0 |= JINGLE_CR0_RUNNING;
        }
    }

    EXIT;
}

static void
__jingle_session_on_terminate(x_object *thiz_)
{
    x_obj_attr_t hints =
    { 0, 0, 0 };
    x_object *_sess_;
    struct jingle_object *thiz = (struct jingle_object *) (void *) thiz_;

    ENTER;
    //    x_object_print_path(o->bus, 0);

    TRACE("Terminating session '%s'\n",_AGET(thiz, "sid"));

    /* just call api */
    if(!(thiz->regs.cr0 & JINGLE_CR0_STOPPED))
    {
        thiz->regs.cr0 |= JINGLE_CR0_STOPPED;
        setattr("sid", (VAL) _AGET(thiz, "sid"), &hints);
        _sess_ = x_session_open(_ENV(thiz, "from"), thiz, &hints, 0);
        x_session_close(_ENV(thiz,"from"),thiz,&hints,0);
    }

    EXIT;
}

static void
__jingle_unknown(x_object *o)
{
}

static void
__jingle_content_accept(x_object *o)
{
}

static void
__jingle_content_add(x_object *o)
{
}

static void
__jingle_content_modify(x_object *o)
{
}

static void
__jingle_content_reject(x_object *o)
{
}

static void
__jingle_content_remove(x_object *o)
{
}
static void
__jingle_description_info(x_object *o)
{
}
static void
__jingle_security_info(x_object *o)
{
}

/**
  * This does everything what ...on_initiate() does.(COPY/PASTED froma there).
  * @todo FIXME make reusable code here!
  */
static void
__jingle_session_on_accept(x_object *o)
{
    x_object *chan;
    x_object *descr;
    x_object *transport;
    x_object *payload;
    x_object *candidate;
    x_object *_chan_;
    x_object *_transport_;
    x_string_t tmpstr1, tmpstr2;
    int err;
    x_obj_attr_t hints = {NULL,NULL,NULL};
    struct jingle_object *thiz = (struct jingle_object *) (void *) o;

    ENTER;

    thiz->regs.cr0 &= ~JINGLE_CR0_PENDING;
    thiz->regs.cr0 |= JINGLE_CR0_RUNNING;

    for (chan = _CHLD(o, "content"); chan; chan = _NXT(chan))
    {
        transport = _CHLD(chan, "transport");
        _chan_ = x_session_channel_open(o, _ENV(chan, "from"), _ENV(chan, "sid"),
                                        _AGET(chan, "name"));

        if (!_chan_)
        {
            TRACE("No such channel: sid(%s),from(%s),name(%s)\n",
                  _ENV(chan, "sid"), _ENV(chan, "from"),
                  _AGET(chan, "name"));
            continue;
        }

        _transport_ = _CHLD(_chan_, "transport");

        if (!_transport_)
        {
            TRACE("No transport for channel: sid(%s),from(%s),name(%s)\n",
                  _ENV(chan, "sid"), _ENV(chan, "from"),
                  _AGET(chan, "name"));
            continue;
        }

        _ASGN(_transport_, (x_obj_attr_t *)&transport->attrs);

        TRACE("Accepting : sid(%s),from(%s),name(%s)\n",
              _ENV(chan, "sid"), _ENV(chan, "from"),
              _AGET(chan, "name"));

        for (candidate = _CHLD(transport, "candidate"); candidate; candidate =
             _NXT(candidate))
        {
            /* set username/password attributes for each candidate
           * for ICE/STUN object */
            tmpstr1 = _ENV(candidate,"ufrag");
            tmpstr2 = _ENV(candidate,"pwd");

            TRACE("ADDING REMOTE CANDIDATE '%s:%s'\n",
                  _AGET(candidate, _XS("ip")),
                  _AGET(candidate, _XS("port")));

            _ASET(candidate, _XS("username"), tmpstr1);
            _ASET(candidate, _XS("password"), tmpstr2);

            x_session_channel_add_transport_candidate(_chan_, candidate);
        }


        /**
       * Iterate over media profiles
       */
        descr = _CHLD(chan, "description");

        for (payload = _CHLD(descr, _XS("payload-type")); payload;
             payload = _NXT(payload))
        {
            x_object *tmp;
            x_object *tmpparam;

            const char *ptname;
            ptname = _AGET(payload,_XS("name"));
            TRACE("ADDING payload '%s'...\n", ptname);

            // assign parameters
            for (tmpparam = _CHLD(payload,_XS("parameter")); tmpparam;
                 tmpparam = _NXT(tmpparam))
            {
                setattr(_AGET(tmpparam,"name"), _AGET(tmpparam,"value"),
                        &payload->attrs);
            }

            //            setattr(_AGET(tmpparam,"mode"), _AGET(tmpparam,"in"),&payload->attrs);
            err = x_session_add_payload_to_channel(_chan_,
                                                   _AGET(payload,_XS("id")),
                                                   ptname, MEDIA_IO_MODE_IN, &payload->attrs);
            if (err < 0)
                TRACE("Cannot set media profile\n");
        }

    }

    EXIT;

}

static void
__jingle_session_info(x_object *o)
{
}

static void
__jingle_transport_accept(x_object *o)
{
}

static void
__jingle_transport_reject(x_object *o)
{
}
static void
__jingle_transport_replace(x_object *o)
{
}

typedef enum
{
    JINGLE_UNKNOWN_TYPE,
    JINGLE_CONTENT_ACCEPT,
    JINGLE_CONTENT_ADD,
    JINGLE_CONTENT_MODIFY,
    JINGLE_CONTENT_REJECT,
    JINGLE_CONTENT_REMOVE,
    JINGLE_DESCRIPTION_INFO,
    JINGLE_SECURITY_INFO,
    JINGLE_SESSION_ACCEPT,
    JINGLE_SESSION_INFO,
    JINGLE_SESSION_INITIATE,
    JINGLE_SESSION_TERMINATE,
    JINGLE_TRANSPORT_ACCEPT,
    JINGLE_TRANSPORT_INFO,
    JINGLE_TRANSPORT_REJECT,
    JINGLE_TRANSPORT_REPLACE,
} x_jingle_action_type;

typedef struct
{
    const char *name;
    void
    (*handler)(x_object *);
} x_jingle_action_t;

static const x_jingle_action_t jingle_actions[] =
{
    { _XS("unknown-type"), __jingle_unknown },
    { "content-accept", __jingle_content_accept },
    { "content-add", __jingle_content_add },
    { "content-modify", __jingle_content_modify },
    { "content-reject", __jingle_content_reject },
    { "content-remove", __jingle_content_remove },
    { "description-info", __jingle_description_info },
    { "security-info", __jingle_security_info },
    { "session-accept", __jingle_session_on_accept },
    { "session-info", __jingle_session_info },
    { "session-initiate", __jingle_session_on_initiate },
    { "session-terminate", __jingle_session_on_terminate },
    { "transport-accept", __jingle_transport_accept },
    { "transport-info", __jingle_session_on_transport_info },
    { "transport-reject", __jingle_transport_reject },
    { "transport-replace", __jingle_transport_replace }, };

x_string_t
jingle_get_action_name(x_jingle_action_type action)
{
    return jingle_actions[action].name;
}

x_jingle_action_type
jingle_get_action_type(const x_string_t action)
{
    static const int num_actions = sizeof(jingle_actions)
            / sizeof(x_jingle_action_t);
    int i = 1;
    for (; i < num_actions; ++i)
    {
        if (EQ(action, jingle_actions[i].name))
            return i;
    }
    return JINGLE_UNKNOWN_TYPE;
}

static int
jingle_msg_recv(x_object *thiz_, const x_object *from, const x_object *msg)
{
    x_string_t subj;
    struct jingle_object *thiz = (struct jingle_object *) (void *) thiz_;
    ENTER;

    TRACE("Got message from %s\n", _GETNM(from));
    subj = _AGET(msg,_XS("subject"));
    TRACE("Subject: %s\n",subj);

    if (EQ(_XS("candidate-new"),subj))
    {
        TRACE("NEW TRANSPORT CANDIDATE RECEIVED '%s:%s'\n",
              _GETNM(from), _AGET(from,_XS("sid")));
        //      x_object_print_path(msg, 0);
        if ((thiz->regs.cr0 != 0))
            __jingle_session_send_candidate(thiz_, from, msg);
    }
#if 0
    else if (EQ(_XS("channel-ready"),subj))
    {
        const char *chnam = _AGET(msg,_XS("chname"));

        TRACE("NEW MEDIA CHANNEL RECEIVED '%s:%s', chname='%s'\n",
              _GETNM(from), _AGET(from,_XS("sid")), chnam);

        //        if (chnam)
        //        {
        //            TRACE("next chan = %p\n", jnglo->pending_channels.next);
        //            delattr(chnam, &jnglo->pending_channels);
        //            if (attr_list_is_empty(&jnglo->pending_channels))
        //            {
        //                TRACE("Initiating new session...\n");
        //                if (jnglo->regs.state == JINGLE_STATE_OUTCOMING)
        //                    __jingle_session_send_initiate(to);
        //                else if (jnglo->regs.state == JINGLE_STATE_INCOMING)
        //                    __jingle_session_send_accept(to);
        //            }
        //        }
    }
#endif
    else if (EQ(_XS("session-activate"),subj))
    {
        if (thiz->regs.state == JINGLE_STATE_OUTCOMING)
        {
            __jingle_session_send_initiate(thiz_);
            __jingle_session_send_transport_info(thiz_);
        }
        else if (thiz->regs.state == JINGLE_STATE_INCOMING)
        {
            __jingle_session_send_transport_info(thiz_);
            /* jingle_state_step */
            {
                if(thiz->regs.cr0 & JINGLE_CR0_PENDING
                        && thiz->regs.cr0 & JINGLE_CR0_REMOTE_CANDIDATE)
                {
                    __jingle_session_send_accept(thiz_);
                    thiz->regs.cr0 &= ~JINGLE_CR0_PENDING;
                    thiz->regs.cr0 |= JINGLE_CR0_RUNNING;
                }
            }
        }
    }
    else if (EQ(_XS("session-destroy"),subj))
    {
        TRACE("Sending terminate (session closed by us) ...\n");
        if(!(thiz->regs.cr0 & JINGLE_CR0_STOPPED))
        {
            thiz->regs.cr0 |= JINGLE_CR0_STOPPED;
            __jingle_session_send_terminate(thiz_);
        }

//        BUG();
        _USBSCRB(from,thiz_);
        thiz->iosession = NULL;
        _REFPUT(thiz_,NULL);
    }
    TRACE("\n");
    EXIT;
    return 0;
}

static void
jingle_state_step(struct jingle_object *thiz)
{
//    JINGLE_CR0_REMOTE_CANDIDATE;
}

static void
jingle_parse_commit(x_object *thiz)
{
    x_jingle_action_type atyp;
    x_object *tmp;
    x_obj_attr_t hints =
    { NULL, NULL, NULL, };
    x_string_t action = NULL;
//    struct x_bus *bus = (struct x_bus *) (void *) thiz->bus;

    ENTER;

    printf("%s:%s():%d\n", __FILE__, __FUNCTION__, __LINE__);

    if (EQ(_XS("error"),_ENV(thiz,_XS("type"))))
    {
        goto _clear_the_state;
    }

    // always ack
    setattr("#type", "result", &hints);
    x_object_send_down(x_object_get_parent(thiz), NULL, &hints);
    attr_list_clear(&hints);

    action = _AGET(thiz, "action");
    atyp = jingle_get_action_type(action);

    TRACE("JINGLE ACTION = '%s'\n", jingle_get_action_name(atyp));

//    sleep(1);

    if (jingle_actions[atyp].handler)
        jingle_actions[atyp].handler(thiz);

    _clear_the_state:
        // clear
        while ((tmp = _CHLD(thiz, NULL)) != NULL)
    {
        _REFPUT(tmp, NULL);
    }

    EXIT;
}

static struct xobjectclass jingle_class;

static void
___jingle_on_session_new(void *_obj)
{
    x_object *stream, *iqo, *xbus;
    struct jingle_object *jingle;
    const char *sid;
    x_obj_attr_t hints =
    { 0, 0, 0 };
    char sbuf[256];

    ENTER;
    x_object_to_path(_obj, sbuf, sizeof(sbuf) - 1);
    TRACE("New session object at '%s'\n", sbuf);

    sid = _AGET(_obj,"sid");
    if (sid)
    {
        xbus = ((x_object *) _obj)->bus;
        stream = _CHLD(xbus,"stream:stream");
        setattr("from", _AGET(X_OBJECT(_obj),"remote"), &hints);
        iqo = _FIND(_CHLD(stream,"iq"), &hints);

        if (!iqo)
        {
            iqo = _GNEW("iq","jabber:client");
            _INS(stream, iqo);
            /* assign attributes */
            _ASGN(iqo, &hints);
        }
        attr_list_clear(&hints);

        setattr("sid", _AGET(X_OBJECT(_obj),"sid"), &hints);
        jingle = (struct jingle_object *) _FIND(
                    _CHLD(iqo,jingle_class.cname), &hints);

        if (!jingle)
        {

            jingle = (struct jingle_object *) _GNEW(jingle_class.cname,JINGLE_NS_1);

            _INS(iqo, X_OBJECT(jingle));
            x_object_assign(X_OBJECT(jingle), &hints);
            TRACE("JINGLE OBJECT NOT FOUND\n");
            //            x_object_print_path(X_OBJECT(jingle), 0);
            jingle->regs.state = JINGLE_STATE_OUTCOMING;

        }
        else
        {
            TRACE("FOUND JINGLE OBJECT:\n");
            //            x_object_print_path(X_OBJECT(jingle), 0);

            jingle->regs.state = JINGLE_STATE_INCOMING;

        }
        attr_list_clear(&hints);

        jingle->iosession = _obj;
        TRACE("Subscribing as %p to %p\n", jingle, _obj);
        _SBSCRB(X_OBJECT(_obj), X_OBJECT(jingle));
        _REFGET(X_OBJECT(_obj));
    }

    EXIT;
}

static struct x_path_listener jingle_session_listener;

static void
___jingle_on_channel_new(void *_obj)
{
    char sbuf[256];
    ENTER;
    x_object_to_path(_obj, sbuf, sizeof(sbuf) - 1);
    TRACE("New channel object at '%s'\n", sbuf);
    EXIT;
}

static struct x_path_listener jingle_channel_listener;

__x_plugin_visibility
__plugin_init void
jingle_init(void)
{
    ENTER;

    jingle_class.cname = "jingle";
    jingle_class.psize = (unsigned int) (sizeof(struct jingle_object)
                                         - sizeof(x_object));
    jingle_class.on_create = &jingle_on_create;
    jingle_class.match = &jingle_match;
    jingle_class.rx = &jingle_msg_recv;
    jingle_class.commit = &jingle_parse_commit;
    x_class_register_ns(jingle_class.cname, &jingle_class, JINGLE_NS_1);

    jingle_session_listener.on_x_path_event = &___jingle_on_session_new;
    x_path_listener_add("__isession", &jingle_session_listener);

    jingle_channel_listener.on_x_path_event = &___jingle_on_channel_new;
    x_path_listener_add("__iostream", &jingle_channel_listener);

    EXIT;
}

PLUGIN_INIT(jingle_init);

