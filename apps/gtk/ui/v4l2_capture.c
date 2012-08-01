/**
 * v4l2_capture.c
 *
 *  Created on: May 9, 2012
 *      Author: artemka
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

void
YUYV_to_i420p(unsigned char *from, unsigned char *to, int w, int h)
{
  unsigned char *luma;
  unsigned char *chromaU;
  unsigned char *chromaV;
  int i, j, k;
  int w2 = w >> 1;
  int h2 = h >> 1;
  int stride = w * 2; /* w * YUYV */

  luma = &to[0];
  chromaU = &to[w * h];
  chromaV = &chromaU[w2 * h2];

  fprintf(stderr, "%s(): w=%d,h=%d\n", __FUNCTION__, w, h);

  for (j = 0; j < h; j++)
    {
      for (i = 0; i < w; i++)
        {
          luma[j * w + i] = from[j * stride + i * 2];

          if (!(j & 0x1)) /* skip odd rows of chroma */
            {
              if (!(i & 0x1)) /* switch chroma planes */
                chromaU[j >> 1 * w2 + i >> 1] = from[j * stride + i * 2 + 1];
              else
                chromaV[j >> 1 * w2 + i >> 1] = from[j * stride + i * 2 + 1];
            }

        }
    }
}

void
nv21_to_i420p(unsigned char *chromaFROM, unsigned char *chromaTO, int w, int h)
{
  unsigned char *chromaU;
  unsigned char *chromaV;
  int i, j, k;
  int w2 = w >> 1;
  int h2 = h >> 1;

  chromaU = &chromaTO[0];
  chromaV = &chromaTO[w2 * h2];

  for (j = 0; j < h; j++)
    {
      for (i = 0; i < w; i++)
        {
          chromaU[j / 2 * w / 2 + i / 2] = chromaFROM[j / 2 * w + i];
          chromaV[j / 2 * w / 2 + i / 2] = chromaFROM[j / 2 * w + i + 1];
        }
    }
}

int
v4l2_next_frame(int fd)
{
  struct v4l2_buffer buf;

  memset(&(buf), 0, sizeof(struct v4l2_buffer));

  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf))
    {
      switch (errno)
        {
      case EAGAIN:
        fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
        return 1;

      case EIO:
        fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
        return -1;
      case EINVAL:
        fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
        return -1;
      case ENOMEM:
        fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
        return -1;
      default:
        perror("Getting video frame\n");
        fprintf(stderr, "%d %s():%d\n", fd, __FUNCTION__, __LINE__);
        return -1;
        }
    }

  if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
    {
      fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
      return -1;
    }

  return 0;
}

int
v4l2_capture_init(unsigned char **_buf, int *w, int *h)
{
  int index;
  int vfd;
  struct v4l2_buffer vbuf;
  struct v4l2_format fmt;

  struct v4l2_buffer qbuf;
  struct v4l2_requestbuffers req;

  enum v4l2_buf_type type;

  fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);

  if ((vfd = open("/dev/video0", O_RDONLY)) < 0)
    {
      perror("Can't open video device\n");
      return -1;
    }

  index = 0;
  if (-1 == ioctl(vfd, VIDIOC_S_INPUT, &index))
    {
      perror("Can't set input\n");
      return -1;
    }

  memset(&(fmt), 0, sizeof(struct v4l2_format));

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = *w;
  fmt.fmt.pix.height = *h;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
//  fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

  if (-1 == ioctl(vfd, VIDIOC_S_FMT, &fmt))
    {
      perror("Can't set video format\n");
      fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
      return -1;
    }

  memset(&(req), 0, sizeof(struct v4l2_requestbuffers));

  req.count = 1;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (-1 == ioctl(vfd, VIDIOC_REQBUFS, &req))
    {
      if (EINVAL == errno)
        {
          fprintf(stderr, "%s does not support "
              "memory mapping\n", "/dev/video0");
          return -1;
        }
      else
        {
          perror("Can't set video VIDIOC_REQBUFS\n");
          fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
          return -1;
        }
    }

  memset(&(vbuf), 0, sizeof(struct v4l2_buffer));

  vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vbuf.memory = V4L2_MEMORY_MMAP;
  vbuf.index = 0;

  if (-1 == ioctl(vfd, VIDIOC_QUERYBUF, &vbuf))
    {
      perror("Can't set video VIDIOC_QUERYBUF\n");
      fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
      return -1;
    }

  *_buf = mmap(NULL, vbuf.length, PROT_READ, MAP_SHARED, vfd, vbuf.m.offset);

  fprintf(stderr, "%s():%d MAPPED v4l AT ADDR=%p len=%d, offset=%p, [%dx%d]\n",
      __FUNCTION__, __LINE__, *_buf, vbuf.length, vbuf.m.offset,
      fmt.fmt.pix.width, fmt.fmt.pix.height);

  memset(&(qbuf), 0, sizeof(struct v4l2_buffer));

  qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  qbuf.memory = V4L2_MEMORY_MMAP;
  qbuf.index = 0;

  if (-1 == ioctl(vfd, VIDIOC_QBUF, &qbuf))
    {
      perror("Can't set video VIDIOC_QBUF\n");
      fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
      return -1;
    }

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == ioctl(vfd, VIDIOC_STREAMON, &type))
    {
      perror("Can't set video VIDIOC_STREAMON\n");
      fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
      return -1;
    }

  return vfd;
}

