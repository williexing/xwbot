#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


static char const hexcmap[] = "0123456789abcdef";

char *
bin2hex(char *buf, int len)
{
    int i;
    char c1,c2;
    char *hex = NULL;

    hex = (char *)malloc(len * 2 + 1);
    if (!hex)
    {
        printf("ERROR!! Fatal len(%d)\n",len);
        return hex;
    }

    for (i = 0; i < len; i++)
    {
        c1 = (char)((buf[i] >> 4) & 0xf);
        c2 = (char)(buf[i] & 0xf);

        hex[2*i] = hexcmap[c1];
        hex[2*i + 1] = hexcmap[c2];
    }

    hex[len * 2] = '\0';
    return hex;
}

int main (int argc, char **argv)
{
    int len;
    int fd;
    char buf[512];
    char *hexstr = NULL;

    fd = open(argv[1],O_RDONLY);

    len = read(fd,buf,sizeof(buf));
    printf("Read %d bytes from stream\n",len);

    hexstr = bin2hex(buf,len);
    printf("---\n%s\n---\n",hexstr);

    close(fd);

    if(hexstr) free(hexstr);

    return 0;
}
