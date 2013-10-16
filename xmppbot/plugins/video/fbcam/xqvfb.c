/**
 * xqvfb.c
 *
 *  Created on: Jul 4, 2012
 *      Author: artemka
 *
 * @author CrossLine Media, Ltd.
 */

#undef DEBUG_PRFX
#include <xwlib/x_config.h>
#if TRACE_VIRTUAL_DIPSLAY_ON
//#define TRACE_LEVEL 2
#define DEBUG_PRFX "[VIRT-QVFB] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <errno.h>

#include "xqvfb.h"

/**
  * FIXME Add thread safety
  * @todo Add thread safety
  */
static int CurrentDID = 0;
static struct ht_cell *sid2did[HTABLESIZE];

static int
openPipe(const char *fileName)
{
    int fd;
    unlink(fileName);
    mkfifo(fileName, 0666);
    fd = open(fileName, O_RDWR | O_NDELAY);
    return fd;
}

static int
__init_key_pipe(int display)
{
    int fd;
    char buf[MAX_PIPE_NAMELEN];
    sprintf(buf, QT_VFB_KEYBOARD_PIPE, display);
    fd = openPipe(buf);
    if (fd == -1)
    {
        printf("Cannot open mouse pipe %s", buf);
        return(-1);
    }
    return fd;
}

static int
__init_mouse_pipe(int display)
{
    int fd;
    char buf[MAX_PIPE_NAMELEN];
    sprintf(buf, QT_VFB_MOUSE_PIPE, display);
    fd = openPipe(buf);
    if (fd == -1)
    {
        printf("Cannot open mouse pipe %s", buf);
        return(-1);
    }
    return fd;
}


static int
qvfb_check_fs_id(int id)
{
    struct stat statbuf;
    char fname[512];
    TRACE("Checking display ID = %d\n",id);

    sprintf(fname, QT_VFB_DATADIR, id);
    if ((stat(fname, &statbuf) == -1) && errno == ENOENT)
    {
        TRACE("Found free display ID = %d\n",id);
        mkdir(fname,S_IRWXU);
        return id;
    }

    return -1;
}

static QVFbSession *
qvfb_get_sess(KEY sid)
{
    int i;
    int idx;
    QVFbSession *qsess = NULL;
    int _did1,_did2;

    if (!sid)
    {
        TRACE("Invalid parameter\n");
        return -EINVAL;
    }

    qsess = ht_get((KEY)sid,(struct ht_cell **)&sid2did,&idx);
    if (!qsess)
    {
        qsess = x_malloc(sizeof(QVFbSession));
        BUG_ON(!qsess);

        _did2 = ++CurrentDID;
        do
        {
            _did1 = qvfb_check_fs_id(_did2++);
        }
        while (_did1 <= 0);

        CurrentDID = _did1;
        qsess->dId = _did1;
        qsess->mousePipe = __init_mouse_pipe(_did1);
        qsess->kbdPipe = __init_key_pipe(_did1);

        ht_set((KEY)sid,(VAL)qsess,(struct ht_cell **)&sid2did);
    }
    return qsess;
}

int
qvfb_get_key_pipe(KEY sid)
{
    QVFbSession *qvfsess = qvfb_get_sess(sid);
    BUG_ON(!qvfsess);
    return qvfsess->kbdPipe;
}

int
qvfb_get_mouse_pipe(KEY sid)
{
    QVFbSession *qvfsess = qvfb_get_sess(sid);
    BUG_ON(!qvfsess);
    return qvfsess->mousePipe;
}

QVFbSession *
qvfb_create(KEY sid, int w, int h, int depth)
{
    QVFbSession *qvfsess = NULL;
    struct QVFbHeader *hdr;

    char oldPipe[512];
    char dataDir[MAX_PIPE_NAMELEN];
    struct sembuf sops;
    int oldPipeSemkey;
    int oldPipeLockId;
    int displaySize;
    int shmId;
    struct shmid_ds shm;
    int rv;
    int bpl;
    unsigned char *dataCache;
    key_t key;
    int lockId = -1;

    /* find/allocate qvfb display session */
    qvfsess = qvfb_get_sess(sid);
    if (!qvfsess)
    {
        TRACE("Cannot create QVFB session\n");
        return NULL;
    }

    /* create data directory */
    sprintf(oldPipe, QTE_PIPE_QVFB, qvfsess->dId, qvfsess->dId);
    oldPipeSemkey = ftok(oldPipe, 'd');
    if (oldPipeSemkey != -1)
    {
        oldPipeLockId = semget(oldPipeSemkey, 0, 0);
        if (oldPipeLockId >= 0)
        {
            sops.sem_num = 0;
            sops.sem_op = 1;
            sops.sem_flg = SEM_UNDO;
            do
            {
                rv = semop(lockId, &sops, 1);
            }
            while (rv == -1 && errno == EINTR);

            perror("QShMemViewProtocol");
            printf("Cannot create lock file as an old version of QVFb has "
                   "opened %s. Close other QVFb and try again", oldPipe);
            exit(-1);
        }
    }

    /*
      * QT_VFB_MOUSE_PIPE already exists after calling
      * to qvfb_get_sess()
      */
    sprintf(dataDir, QT_VFB_MOUSE_PIPE, qvfsess->dId);
    key = ftok(dataDir, 'g');

    if (depth < 8)
        bpl = (w * depth + 7) / 8;
    else
        bpl = w * ((depth + 7) / 8);

    displaySize = bpl * h;

    unsigned char *data;
    uint data_offset_value = sizeof(struct QVFbHeader);

    int dataSize = bpl * h + data_offset_value;

    printf("\n\tmapped %d of something,\n\t"
           "width=%d\n\t"
           "height=%d\n\t"
           "data_offset_value=%d,\n\t"
           "stride=%d\n\t"
           "key=%d\n", dataSize, w, h, data_offset_value, bpl, key);

    shmId = shmget(key, dataSize, IPC_CREAT | 0666);
    if (shmId != -1)
    {
        data = (unsigned char *) shmat(shmId, 0, 0);
    }
    else
    {
        shmctl(shmId, IPC_RMID, &shm);
        shmId = shmget(key, dataSize, IPC_CREAT | 0666);
        if (shmId == -1)
        {
            perror("QShMemViewProtocol::QShMemViewProtocol");
            printf("Cannot get shared memory 0x%08x", key);
            exit(-1);
        }
        data = (unsigned char *) shmat(shmId, 0, 0);
    }

    if ((long) data == -1)
    {
        /*
       * @todo Delete key/mouse pipes
       */
        perror("QShMemViewProtocol::QShMemViewProtocol");
        printf("Cannot attach to shared memory %d", shmId);
        exit(-1);
    }

    dataCache = (unsigned char *) malloc(displaySize);
    memset(dataCache, 0, displaySize);

    memset(data + sizeof(struct QVFbHeader), 0, displaySize);

    hdr = (struct QVFbHeader *) data;
    hdr->width = w;
    hdr->height = h;
    hdr->depth = depth;
    hdr->linestep = bpl;
    hdr->dataoffset = data_offset_value;
    hdr->update_x1 = 0;
    hdr->update_y1 = 0;
    hdr->update_x2 = w;
    hdr->update_y2 = h;
    hdr->dirty = 0;
    hdr->numcols = 0;
    hdr->viewerVersion = 0x040703; // QT_VERSION 4.7.3
    hdr->brightness = 255;
    hdr->windowId = 0;

    qvfsess->displayMem = (struct QVFbHeader *)data;
    return qvfsess;
}

#if 0
int
main(int argc, char **argv)
{
    int fd;
    unsigned char *data;
    data = qvfb_create(0, 320, 240, 24);
    getchar();
    fd = open("screen.dmp", O_RDWR | O_NDELAY | O_CREAT | O_TRUNC, S_IRWXU);
    write(fd, data + sizeof(struct QVFbHeader), 320 * ((24 + 7) / 8) * 240);
    close(fd);
}
#endif
