/**
 * xqvfb.c
 *
 *  Created on: Jul 4, 2012
 *      Author: artemka
 */

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

#include "xqvfb.h"

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

unsigned char *
qvfb_create(int displayid, int w, int h, int depth)
{
  struct QVFbHeader *hdr;
  const char * username = "unknown";

  char oldPipe[512];
  char displayPipe[MAX_PIPE_NAMELEN];
  char mousePipe[MAX_PIPE_NAMELEN];

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
  const char *logname = getenv("LOGNAME");

  if (logname)
    username = logname;

  /* create data directory */
  sprintf(oldPipe, QTE_PIPE_QVFB, displayid, displayid);

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

  __init_key_pipe(displayid);
  __init_mouse_pipe(displayid);

  sprintf(displayPipe, QTE_PIPE_QVFB, displayid, displayid);
  sprintf(mousePipe, QT_VFB_MOUSE_PIPE, displayid);

  key = ftok(mousePipe, 'g');

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

  return data;
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
