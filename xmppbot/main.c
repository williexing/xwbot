#undef DEBUG_PRFX
#define DEBUG_PRFX "[xmppbot] "
#include <xwlib/x_types.h>
#include <xmppagent.h>

#ifndef WIN32
#include <dirent.h>
#ifndef  IOS
#include <dlfcn.h>
#endif
#endif

#include <plugins/sessions_api/sessions.h>

#include <xbot_config.h>

#if defined(ANDROID)
#       define PLUGINS_DIR "/data/data/com.xw/lib"
#endif

// @todo this should be moved into xwlib
#ifdef DEBUG_CALLSTACK
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>

/* This structure mirrors the one found in /usr/include/asm/ucontext.h */
typedef struct _sig_ucontext
{
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
}sig_ucontext_t;

static void
crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext)
{
    void * array[50];
    void * caller_address;
    char ** messages;
    int size, i;
    sig_ucontext_t * uc;

    uc = (sig_ucontext_t *) ucontext;

    /* Get the address at the time the signal was raised from the EIP (x86) */
    caller_address = (void *) uc->uc_mcontext.rip;

    fprintf(stderr, "signal %d (%s), address is %p from %p\n", sig_num,
            strsignal(sig_num), info->si_addr, (void *) caller_address);

    size = backtrace(array, 50);

    /* overwrite sigaction with caller's address */
    array[1] = caller_address;

    messages = backtrace_symbols(array, size);

    /* skip first stack frame (points here) */
    for (i = 1; i < size && messages != NULL; ++i)
    {
        fprintf(stderr, "[bt]: (%d) %s\n", i, messages[i]);
    }

    free(messages);

    exit(EXIT_FAILURE);
}

static void
register_sighandler(void)
{
    struct sigaction sigact;
    ENTER;

    sigact.sa_sigaction = &crit_err_hdlr;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction(SIGSEGV, &sigact, (struct sigaction *) NULL) != 0)
    {
        fprintf(stderr, "error setting signal handler for %d (%s)\n", SIGSEGV,
                strsignal(SIGSEGV));
        exit(EXIT_FAILURE);
    }
}
#endif /* DEBUG_CALLSTACK */

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
#elif !defined(IOS) /* WIN32 */
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

/**
 *
 */
EXPORT_SYMBOL x_object *
gobee_init(int argc, char **argv)
{
    x_object *bus;
    x_obj_attr_t attrs =
    { 0, 0, 0 };

    ENTER;

#ifdef DEBUG_CALLSTACK
    register_sighandler();
#endif

    /* load all additional plugins from plugins dir */
#if !defined(IOS)
    gobee_load_plugins(PLUGINS_DIR);
    gobee_load_plugins("./");
    gobee_load_plugins("./output");
#endif
  
    /* instanciate '#xbus' object from 'gobee' namespace */
    bus = x_object_new_ns("#xbus", "gobee");
    BUG_ON (!bus);

    TRACE("Start processing....\n");

    /* parse command line */
    if (argc <= 2)
    {
        TRACE("Invalid params\n");
        return (NULL);
    }

    /* two ways on setting xbus attributes */
#if 0
    x_object_setattr(bus, "pword", argv[2]);
    x_object_setattr(bus, "jid", argv[1]);
#else
    setattr("pword", argv[2], &attrs);
    setattr("jid", argv[1], &attrs);
    x_object_assign(bus, &attrs);
#endif

    EXIT;
    return bus;
}

EXPORT_SYMBOL void
gobee_connect(x_object *bus)
{
    /* start xbus */
    bus->objclass->commit(bus);
}


static void
gobee_place_call(x_object *obj, const char *jid)
{
    x_object *_chan_;
    x_object *_sess_ = NULL;
    x_obj_attr_t hints =
    { NULL, NULL, NULL, };

    BUG_ON(!jid);

    _sess_ = x_session_open(jid, X_OBJECT(obj), &hints, X_CREAT);
    if (!_sess_)
    {
        BUG();
    }

#if 1
    /* open/create audio channel */
    _chan_ = x_session_channel_open2(X_OBJECT(_sess_), "m.A");

    setattr(_XS("mtype"), _XS("audio"), &hints);
    _ASGN(X_OBJECT(_chan_), &hints);

    attr_list_clear(&hints);
    x_session_channel_set_transport_profile_ns(X_OBJECT(_chan_), _XS("__icectl"),
                                               _XS("jingle"), &hints);
    x_session_channel_set_media_profile_ns(X_OBJECT(_chan_),
                                           _XS("__rtppldctl"),_XS("jingle"));

    /* add channel payloads */
    setattr("clockrate", "16000", &hints);
    x_session_add_payload_to_channel(_chan_, _XS("110"), _XS("SPEEX"), MEDIA_IO_MODE_OUT, &hints);
    /* clean attribute list */
    attr_list_clear(&hints);
#endif

#if 1
    /* open/create video channel */
    _chan_ = x_session_channel_open2(X_OBJECT(_sess_), "m.V");

    setattr(_XS("mtype"), _XS("video"), &hints);
    _ASGN(X_OBJECT(_chan_), &hints);

    attr_list_clear(&hints);
    x_session_channel_set_transport_profile_ns(X_OBJECT(_chan_), _XS("__icectl"),
                                               _XS("jingle"), &hints);

    x_session_channel_set_media_profile_ns(X_OBJECT(_chan_), _XS("__rtppldctl"),_XS("jingle"));

    /* add channel payloads */
    attr_list_clear(&hints);
    setattr(_XS("clockrate"), _XS("90000"), &hints);
    setattr(_XS("width"), _XS("320"), &hints);
    setattr(_XS("height"), _XS("240"), &hints);
    setattr(_XS("sampling"), _XS("YCbCr-4:2:0"), &hints);

    x_session_add_payload_to_channel(_chan_, _XS("96"), _XS("THEORA"), MEDIA_IO_MODE_OUT, &hints);

    attr_list_clear(&hints);
#endif

#if 1
    /* open/create hid channel */
    _chan_ = x_session_channel_open2(X_OBJECT(_sess_), "m.H");

    setattr(_XS("mtype"), _XS("application"), &hints);
    _ASGN(X_OBJECT(_chan_), &hints);

    attr_list_clear(&hints);
    x_session_channel_set_transport_profile_ns(X_OBJECT(_chan_), _XS("__icectl"),
                                               _XS("jingle"), &hints);
    x_session_channel_set_media_profile_ns(X_OBJECT(_chan_), _XS("__rtppldctl"),_XS("jingle"));
    x_session_add_payload_to_channel(_chan_, _XS("101"), _XS("HID"), MEDIA_IO_MODE_OUT, &hints);
#endif

    // commit channel
    setattr(_XS("$commit"), _XS("yes"), &hints);
    _ASGN(X_OBJECT(_chan_), &hints);
    attr_list_clear(&hints);

    x_session_start(_sess_);
}


#if (!defined(ANDROID)) && (!defined(IOS))
#ifdef WIN32
#include <tchar.h>
int _tmain (int argc, TCHAR *argv[])
#else
int main (int argc, char *argv[])
#endif
{
    THREAD_T tbus_t;
    x_object *bus;
#ifdef _WIN32
    {
        WORD wVersionRequested;
        WSADATA wsaData;
        int err;
	wVersionRequested = MAKEWORD(2, 2);
        err = WSAStartup(wVersionRequested, &wsaData);
        BUG_ON(err);
    }
#endif

    bus = gobee_init(argc, argv);
    x_thread_run(&tbus_t, (t_func)gobee_connect, (void *)bus);

    sleep(5);
    if (argc > 3)
    {
        gobee_place_call(bus, argv[3]);
    }

    TRACE("Running....\n");
    
    for (;;)
        getchar();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
#endif

