/*
 * shell.c
 *
 *  Created on: Jul 28, 2011
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[__inbandshell] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <plugins/sessions_api/sessions.h>

typedef struct inband_shell
{
    x_object xobj;
    int shell_out[2];
    int shell_in[2];

    struct
    {
        int state;
        volatile uint32_t cr0;
    } regs;

    struct ev_loop *shellevtloop;
    struct ev_io shell_watcher;

#ifndef __DISABLE_MULTITHREAD__
    THREAD_T shell_tid;
#endif
    pid_t c_pid;
} inband_shell_t;


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


#if 1
/**
 */
static int
__ibs_shell_chk_nexthop(x_object *thiz)
{
    int err = -1;
    x_object *nxthop;
    x_object *parent = _PARNT(thiz);


    if (!thiz->write_handler &&
            parent)
    {
        nxthop = _CHLD(parent, MEDIA_OUT_CLASS_STR);
        x_object_set_write_handler(thiz, nxthop);
    }

    if (thiz->write_handler)
        err = 0;

    return err;
}

static void
__ibs_shell_on_data_cb(struct ev_loop *loop, struct ev_io *watcher, int mask)
{
    int err = 0;
    inband_shell_t *thiz;
    x_object *tmpo;
    char buf[512];
    //  x_string str = {0,0,0};

    TRACE("some data ready: reading...\n");

    thiz = (inband_shell_t *) (((char *) watcher)
                               - offsetof(inband_shell_t, shell_watcher));

    if (mask & EV_READ)
    {
        TRACE("new data from shell\n");
        if ((err = x_read(thiz->shell_out[0], buf, 511)) > 0)
        {
            buf[err] = '\0';
            TRACE("<%s>\n", buf);
            // write to out$media
            if (thiz->xobj.write_handler
                    || !__ibs_shell_chk_nexthop(thiz))
            {
                _WRITE(thiz->xobj.write_handler, buf, err, NULL);
            }
        }
    }

    return;
}

/**
 *
 */
static int
__ibs_shell_job_new(void *thiz_)
{
    inband_shell_t *thiz = (inband_shell_t *) thiz_;
    int err;
    ENTER;

    err = pipe(thiz->shell_in);
    err = pipe(thiz->shell_out);

    if (!(thiz->c_pid = fork()))
    {
        dup2(thiz->shell_out[1], 1);
        dup2(thiz->shell_out[1], 2);
        dup2(thiz->shell_in[0], 0);

        close(thiz->shell_out[0]);
        close(thiz->shell_in[1]);

        TRACE("Shell started\n");
        execl("/bin/sh", "sh", (char*) NULL);
        exit(1);
    }

    close(thiz->shell_out[1]);
    close(thiz->shell_in[0]);

    fcntl(thiz->shell_out[0], F_SETFL, O_NONBLOCK);
    fcntl(thiz->shell_in[1], F_SETFL, O_NONBLOCK);

    ev_io_init(&thiz->shell_watcher, &__ibs_shell_on_data_cb, thiz->shell_out[0],
            EV_READ /* | EV_WRITE */);

    EXIT;
    return 0;
}

static void *
__ibs_shell_worker(void *thiz_)
{
    inband_shell_t *thiz = (inband_shell_t *) (void *) thiz_;

    ENTER;

    TRACE("Waiting on start condition...\n");
#ifndef CS_DISABLED
    //  CS_LOCK(CS2CS(thiz->lock));
#endif

    // set 'started' flag
    thiz->regs.cr0 &= ~((uint32_t) (1 << 1));

#ifndef __DISABLE_MULTITHREAD__
    do
    {
        TRACE("Started loop thread\n");
        ev_loop(thiz->shellevtloop, 0);
    }
    while (!(thiz->regs.cr0 & (1 << 0)) /* thread stop condition!!! */);
#endif

#ifndef CS_DISABLED
    //  CS_UNLOCK(CS2CS(thiz->lock));
#endif

    EXIT;
    return NULL;
}

static int
__ibs_shell_worker_start(inband_shell_t *thiz)
{
    int err;
    ENTER;

    thiz->shellevtloop = ev_loop_new(EVFLAG_AUTO);
    BUG_ON(!thiz->shellevtloop);

    ev_io_start(thiz->shellevtloop, &thiz->shell_watcher);

#ifndef __DISABLE_MULTITHREAD__
    x_thread_run(&thiz->shell_tid, &__ibs_shell_worker, (void *) thiz);
#endif

    EXIT;
    return err;
}

#if 1
static int
__ibs_shell_worker_stop(inband_shell_t *thiz)
{
    /* cancel working thread */
    thiz->regs.cr0 &= ~((uint32_t) (1 << 0));

    ev_io_stop(thiz->shellevtloop, &thiz->shell_watcher);
    ev_unloop(thiz->shellevtloop,EVUNLOOP_ALL);

#ifdef HAVE_PTHREAD_H
    pthread_cancel(thiz->shell_tid);

    TRACE("Joining SHELL thread\n");
    pthread_join(thiz->shell_tid,NULL);
#endif
}
#endif


static int
inbandshell_on_append(x_object *o, x_object *parent)
{
    x_string_t name;
    inband_shell_t *thiz = (inband_shell_t *) o;

    TRACE("Creating SHELL job\n");
    // create new shell process
    __ibs_shell_job_new(thiz);

    TRACE("Starting SHELL thread\n");
    // start shell job
    __ibs_shell_worker_start(thiz);

    EXIT;
    return 0;
}


static int
inbandshell_try_write(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    int err;
    x_object *tmpo;
    inband_shell_t *shio = (inband_shell_t *) (void *) o;

    TRACE("received message to write %s\n", buf);

    err = x_write(shio->shell_in[1],buf,len);
    err = x_write(shio->shell_in[1],"\n",1);

    return err;
}

static x_object *
inbandshell_on_assign(x_object *thiz, x_obj_attr_t *attrs)
{
    x_object_default_assign_cb(thiz, attrs);
    return thiz;
}

static void
inbandshell_remove_cb(x_object *o)
{
    inband_shell_t *thiz = (inband_shell_t *) (void *) o;
    ENTER;
    __ibs_shell_worker_stop(thiz);
    EXIT;
}

static void UNUSED
inbandshell_release_cb(x_object *o)
{
    ENTER;
    EXIT;
}

/* matching function */
static BOOL
inbandshell_equals(x_object *o, x_obj_attr_t *attrs)
{
    return TRUE;
}

static struct xobjectclass __ibshell_class;

__x_plugin_visibility
__plugin_init void
__inbandshell_init(void)
{
    ENTER;
    __ibshell_class.cname = X_STRING("__ibshell");
    __ibshell_class.psize = (unsigned int) (sizeof(inband_shell_t)
                                            - sizeof(x_object));

    __ibshell_class.match = &inbandshell_equals;
    __ibshell_class.on_append = &inbandshell_on_append;
    __ibshell_class.on_assign = &inbandshell_on_assign;
    __ibshell_class.on_remove = &inbandshell_remove_cb;
    __ibshell_class.on_release = &inbandshell_release_cb;
    __ibshell_class.try_write = &inbandshell_try_write;

    x_class_register_ns(__ibshell_class.cname, &__ibshell_class,
                        _XS("gobee:media"));
    EXIT;
    return;
}
PLUGIN_INIT(__inbandshell_init);

#endif
