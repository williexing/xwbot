/*
 * session_tester.c
 *
 *  Created on: Dec 1, 2011
 *      Author: artemka
 */

#include <xwlib/x_types.h>
#include <xwlib/x_lib.h>

#define CLI_ADDR "/tmp/clisock"

static struct sockaddr_un server_address;
static struct sockaddr_un client_address;

/*
 * Creates external IO communication slot.
 */
static int
__session_create_io_slot(void)
{
  int sockfd;

  sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("client: open socket failed\n");
      return -1;
    }

  x_memset(&client_address, 0, sizeof(struct sockaddr_un));

  client_address.sun_family = AF_UNIX;
  x_strncpy(client_address.sun_path, CLI_ADDR, sizeof(client_address.sun_path)
      - 1);

  if (bind(sockfd, (struct sockaddr *) &client_address, sizeof(struct sockaddr_un))
      == -1)
    {
      close(sockfd);
      printf("client: connection failed\n");
      return -1;
    }

  return sockfd;
}

int
main(int argc, char **argv)
{
  socklen_t alen = sizeof(server_address);
  int siz;
  char buf[128];
  int k = 20;

  int sockfd = __session_create_io_slot();
  if (sockfd > 0)
    {
      memset(&server_address, 0, sizeof(struct sockaddr_un));
      server_address.sun_family = AF_UNIX;
      x_strncpy(server_address.sun_path, argv[1], sizeof(server_address.sun_path)
          - 1);

      do
        {
          fgets(buf, sizeof(buf), stdin);
          sendto(sockfd, buf, strlen(buf), 0,
              (struct sockaddr *) &server_address, alen);

          siz = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
              (struct sockaddr *) &server_address, &alen);

          if (siz > 0)
            {
              buf[siz] = 0;
              printf("%s\n", buf);
            }
        }
      while (1);
    }
}
