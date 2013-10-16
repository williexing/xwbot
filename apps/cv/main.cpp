
#include "featuretrack.h"

#include <x_config.h>
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>
#include <xwlib++/x_objxx.h>

#include <dirent.h>
#include <dlfcn.h>

using namespace gobee;

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

int
main (void)
{
    x_obj_attr_t hints = {0,0,0};

    gobee_load_plugins(".");
    gobee_load_plugins("./output");

    __x_object *rootbus = new ("gobee","#rootbus") __x_object();
    __x_object *camobj = new ("gobee:media","camera") __x_object();

    rootbus->add_child(camobj);

    (*rootbus)+= &hints;
    pause();
}

