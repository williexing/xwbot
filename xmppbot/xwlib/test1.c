#include <stdio.h>
#include <stdlib.h>
#include "x_obj.h"

static void
test1(int i)
{
  x_object *xobj;
  for (; i > 0; i--)
    {
      xobj = _GNEW("testobj",NULL);
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

//  len = x_object_to_path(xobj, pbuf, sizeof(pbuf) - 1);
//  printf("%s\n", pbuf);
//  printf("wrote %d\n", len);

  x_object_print_path(root,0);

//  _REFPUT(root, NULL);
  x_object_destroy_tree(root);
}

static void
test4(void)
{
//    int len;
//    char pbuf[8192 * 4];
    x_object *testo;

    testo = _GNEW("testobj","default");
    x_object_vassign(testo,
                     1,"key1","val1",
                     1,"key2","val2",
                     1,"key3","val3",
                     1,"key4","val4",
                     1,"key5","val5",
                     0);

    x_object_print_path(testo,0);

    _REFPUT(testo, NULL);
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
  test4();
  printf("ok!");
  getchar();
}
