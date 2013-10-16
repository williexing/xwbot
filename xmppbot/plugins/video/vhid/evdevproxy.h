#ifndef EVDEVPROXY_H
#define EVDEVPROXY_H

#include <iostream>
//#include <linux/time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

typedef struct evdevdata
{
    struct timeval t;
    unsigned short type;
    unsigned short code;
    unsigned int val;
} evdevdata;

class EvdevProxy
{
    int evfd[64];
    int fdcount;

public:
    EvdevProxy() : fdcount(0)
    {

    }

    ~EvdevProxy()
    {

    }

    bool
    addSrc(const char *evsrcfile)
    {
        evfd[fdcount++] = open(evsrcfile, O_RDWR);
        return true; // always ok :)
    }

    void
    pushMouseEv(double dx, double dy, unsigned int buttons)
    {

    }

    void
    pushKbdEv(int kcode, int modif)
    {

    }

    void
    readEvent(void)
    {
        int err;
        evdevdata evt;
        if((err = read(evfd[0], &evt, sizeof(evt))) < 0)
        {
            return;
        }
        ::printf("%d bytes -> %lu.%lu: %d: %d = %d\n",err,
                 (unsigned long)evt.t.tv_sec, (unsigned long)evt.t.tv_usec,
                 evt.type, evt.code, evt.val);
    }

    void
    writeEvent(int type, int code, int val)
    {
        int err,_e;
        evdevdata evt;

        gettimeofday(&evt.t,NULL);

        evt.code = code;
        evt.type = type;
        evt.val  = val;

        _e = sizeof(evt);
//        if((err = write(evfd[0], &(evt.type), sizeof(evt) - sizeof(struct timeval))) < 0)
        if((err = write(evfd[0], &evt, _e)) <= 0)
        {
            ::printf("Error!:");
            return;
        }

    }

};

#endif // EVDEVPROXY_H
