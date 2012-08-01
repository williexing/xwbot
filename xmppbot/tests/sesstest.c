#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

struct sockaddr_un server_address;
struct sockaddr_un client_address;

static int
session_sock_open(const char *sname)
{
  int sockfd;

  sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("opening socket failed\n");
      return -1;
    }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sun_family = AF_UNIX;
  strncpy(server_address.sun_path, sname, sizeof(server_address.sun_path) - 1);

#if 0
  memset(&client_address, 0, sizeof client_address);
  client_address.sun_family = AF_UNIX;
  if(bind(sockfd, (const struct sockaddr *) &client_address,
          sizeof(struct sockaddr_un)) < 0)
    {
       printf("bind failed\n");
       return -1;
    }
#else

  if(connect(sockfd, (const struct sockaddr *) &server_address,
      sizeof(struct sockaddr_un)) < 0)
    {
       printf("connect failed\n");
       return -1;
    }
#endif
  return sockfd;
}

int
main(int argc, char *argv[])
{
  int sock = session_sock_open(argv[1]);

#if 1
  sendto(sock, "Hello GoBee!\n", strlen("Hello GoBee!\n"), 0,
      (const struct sockaddr *) &server_address, sizeof(struct sockaddr_un));
#else
  write(sock, "Hello GoBee!\n", strlen("Hello GoBee!\n"));
#endif
  close(sock);
  exit(0);
}
