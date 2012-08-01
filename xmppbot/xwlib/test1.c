#include <stdio.h>
#include <stdlib.h>
#include "x_obj.h"

static void
test1(int i)
{
  x_object *xobj;
  for (; i > 0; i--)
    {
      xobj = _NEW("testobj",NULL);
      _REFPUT(xobj, NULL);
    }
}

static void
test2(int i)
{
  x_object *root, *xobj, *tmp;

  root = x_object_new("testobj");
  xobj = root;
  do
    {
      tmp = x_object_new("testchild");
      x_object_append_child(xobj, tmp);
      xobj = tmp;
    }
  while (--i > 0);

  _REFPUT(root,NULL);

}

static void
test3(int i)
{
  int len;
  char pbuf[8192 * 4];
  x_object *root, *xobj, *tmp;

  root = x_object_new("testobj");
  xobj = root;
  do
    {
      tmp = x_object_new("testchild");
      x_object_append_child(xobj, tmp);
      xobj = tmp;
    }
  while (--i > 0);

  len = x_object_to_path(xobj, pbuf, sizeof(pbuf) - 1);
  printf("%s\n", pbuf);
  printf("wrote %d\n", len);

  _REFPUT(root, NULL);

}

int
main(int argc, char **argv)
{
  int cnt = strtol(argv[1], (char **) NULL, 10);
  test1(cnt);
  printf("ok!");
  getchar();
  test2(cnt);
  printf("ok!");
  getchar();
  test3(cnt);
  printf("ok!");
  getchar();
}
