#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/un.h>

#include <xilib/xilib.h>

int main()
{
  int ver = xilib_get_version();
  xilib_comm_t comm;

  printf("Using xilib %d.%d\n", ver>>16, ver&0xffff);

  if (xilib_open(&comm) < 0)
    {
      printf("xilib_open failed\n");
      return -1;
    }

  if (xilib_test(&comm) < 0)
    {
      printf("xilib_test failed\n");
      xilib_close(&comm);
      return -1;
    }

  xilib_close(&comm);
  return 0;
}
