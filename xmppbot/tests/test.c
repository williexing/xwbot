#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main (void)
{
  int err;
  char buf[24];
  int fd;
  fd = open("callback.log",
      O_CREAT | O_RDWR | O_APPEND | O_SYNC, S_IRUSR | S_IWUSR);
perror(NULL);
printf("FD = %d\n",fd);
  err = write(fd, buf, 10);
 // sync();
//  close(_ifd);
	
printf("Wrote %d bytes\n",err);
perror(NULL);
}

