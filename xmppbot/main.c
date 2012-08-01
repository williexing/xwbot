#undef DEBUG_PRFX
#define DEBUG_PRFX "[xmppbot] "
#include <xwlib/x_types.h>
#include <xmppagent.h>

#ifndef WIN32
#include <dirent.h>
#include <dlfcn.h>
#endif

#include "config.h"

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
  gobee_load_plugins(PLUGINS_DIR);
  gobee_load_plugins("./");

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
  bus->objclass->finalize(bus);
}

#if (!defined(ANDROID)) && (!defined(IOS))
#ifdef WIN32
#include <tchar.h>
int _tmain (int argc, TCHAR *argv[])
#else
int main (int argc, char *argv[])
#endif
{
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
  gobee_connect(bus);
  return 0;
}
#endif

