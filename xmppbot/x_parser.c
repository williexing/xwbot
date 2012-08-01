/*
 * x_parser.c
 *
 *  Created on: Aug 9, 2010
 *      Author: artur
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[parser] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_lib.h>
#include <xwlib/x_obj.h>

#include <x_parser.h>

static int
x_parser_ctx_txt(struct x_push_parser *p, const char c);
static int
x_parser_ctx_tag(struct x_push_parser *p, const char c);
static int
x_parser_ctx_attr(struct x_push_parser *p, const char c);
static int
x_parser_set_context(struct x_push_parser *p, x_putc_ft func);
static int
x_parser_ctx_idle(struct x_push_parser *p, const char c);
static __inline__ void
x_parser_push(struct x_push_parser *p, void *v);
static __inline__ void *
x_parser_pop(struct x_push_parser *p);

/*
 * Push parser
 */
static int
x_parser_ctx_txt(struct x_push_parser *p, const char c)
{
  //  char volatile _c = c;
  if (c == '<')
    {
      /* set next state */
      x_parser_set_context(p, &x_parser_ctx_tag);
    }
  /* text value */
  else
    {
      if (p->xputchar)
        p->xputchar(c, p->cbdata);
    }
  return 0;
}

/**
 * This skips character until closing char appears
 */
static int
x_parser_ctx_idle(struct x_push_parser *p, const char c)
{
  x_putc_ft f;
  if (((u_int32_t) p->g1 & 0xffUL) != (u_int32_t) c)
    {
      if (p->g2)
        {
          x_string_putc((x_string *) p->g2, c);
        }
      return 0;
    }
  else
    {
      f = (x_putc_ft) x_parser_pop(p);
      /* restore state from stack */
      x_parser_set_context(p, f);
    }
  return 0;
}

/*
 * Push parser
 */
static int
x_parser_ctx_tag(struct x_push_parser *p, const char c)
{
  if (p->cr1 == X_PP_INI)
    {
      if (x_isalpha(c) || c == '_' || c == ':' || c < '\0')
        {
          p->cr1 = X_PP_OBJ_OPENING;
          x_string_clear(&p->r0);
          /* append char to tag name */
          x_string_putc(&p->r0, c);
        }
      else if (c == '?')
        {
          p->g1 = (void *) c;
          /* save return state */
          x_parser_push(p, &x_parser_ctx_tag);
          /* skip processing instructions */
          x_parser_set_context(p, &x_parser_ctx_idle);
        }
      else if (c == '>')
        {
          p->cr1 = X_PP_INI;
          /* clear tagname value */
          x_string_clear(&p->r0);
          /* set text context */
          x_parser_set_context(p, &x_parser_ctx_txt);
        }
      else if (c == '/')
        {
            {
              x_string_clear(&p->r1);
              x_string_clear(&p->r2);
              p->g1 = p->g2 = p->g3 = p->g4 = 0;
            }
          p->cr1 = X_PP_OBJ_CLOSING;
        }
      else
        {
          TRACE("Parse error! Invalid character for context: '%c'\n", c);
          return (-1);
        }
    }
  else if (p->cr1 == X_PP_OBJ_OPENING)
    {
      if (x_isalpha(c) || c == '-' || x_isdigit(c) || c == '_' || c == '.'
          || c == ':' || c < '\0')
        {
          /* append char to tag name */
          x_string_putc(&p->r0, c);
        }
      else if (c == '/')
        {
          // TRACE ("1 Allocaing... %s\n",p->r0.cbuf);
          if (p->xobjopen)
            {
              p->xobjopen(p->r0.cbuf, &p->attrs, p->cbdata);
            }
            {
              x_string_clear(&p->r1);
              x_string_clear(&p->r2);
              p->g1 = p->g2 = p->g3 = p->g4 = 0;
            }
          p->cr1 = X_PP_OBJ_CLOSING;
        }
      else if (c == '>')
        {
          if (p->xobjopen)
            p->xobjopen(p->r0.cbuf, &p->attrs, p->cbdata);

            {
              x_string_clear(&p->r0);
              x_string_clear(&p->r1);
              x_string_clear(&p->r2);
              p->g1 = p->g2 = p->g3 = p->g4 = 0;
            }

          p->cr1 = X_PP_INI;
          // unset attr flag
          p->cr0 &= ~PP_ATTR_F;
          /* set text context */
          x_parser_set_context(p, &x_parser_ctx_txt);
        }
      else if ((x_strchr("\t\r\n ", c) != NULL))
        {
          /* create pattern object */
          attr_list_clear(&p->attrs);

            {
              x_string_clear(&p->r1);
              x_string_clear(&p->r2);
              p->g1 = p->g2 = p->g3 = p->g4 = 0;
            }
          /* set next state */
          p->cr0 |= PP_ATTR_F;
          x_parser_set_context(p, &x_parser_ctx_attr);
        }
      else
        {
          EXIT;
          BUG();
        }
    }

  else if (p->cr1 == X_PP_OBJ_CLOSING)
    {
      if (c == '>')
        {
          if (p->xobjclose)
            p->xobjclose(p->r0.cbuf, p->cbdata);

          /* unset state */
          p->cr1 = X_PP_INI;
            {
              /*
               TRACE("PARSER STATE:\n");
               printf("\tr0: %s\n",p->r0.cbuf);
               printf("\tr1: %s\n",p->r1.cbuf);
               printf("\tr2: %s\n",p->r2.cbuf);
               */
              x_string_clear(&p->r0);
              x_string_clear(&p->r1);
              x_string_clear(&p->r2);
              p->g1 = p->g2 = p->g3 = p->g4 = 0;
            }
          x_parser_set_context(p, &x_parser_ctx_txt);
        }
      else if (x_isalpha(c) || c == '-' || x_isdigit(c) || c == '_' || c == '.'
          || c == ':' || c < '\0')
        {
          /* append char to tag name */
          x_string_putc(&p->r0, c);
        }
      else
        {
          TRACE("Parse error! Invalid character for context: '%c'\n", c);
          return (-1);
        }
    }
  else
    {
      TRACE("Parse error! Invalid character for context: '%c'\n", c);
      exit(-1);
    }

  return 0;
}

/*
 * Push parser
 */
static int
x_parser_ctx_attr(struct x_push_parser *p, const char c)
{
  if (p->cr1 == X_PP_KV_LAV)
    {
      if (x_isalpha(c) || c == '_' || c == ':' || c < '\0')
        {
          x_string_putc(&p->r2, c);
        }
      else if ((x_strchr("\t\r\n ", c) != NULL))
        {
          setattr(p->r1.cbuf, p->r2.cbuf, &p->attrs);
          //         TRACE ("Setting attribute '%s'='%s'\n", p->r1.cbuf, p->r2.cbuf);

          x_string_clear(&p->r1);
          x_string_clear(&p->r2);

          /* clear flags */
          p->cr1 = X_PP_INI;
        }
      else
        {
          setattr(p->r1.cbuf, p->r2.cbuf, &p->attrs);
          //         TRACE ("Setting attribute '%s'='%s'\n", p->r1.cbuf, p->r2.cbuf);

          goto ret_to_tag;
        }
    }
  /* in the 'value' phase */
  else if (p->cr1 == X_PP_KV_VAL)
    {
      if ((x_strchr("\t\r\n ", c) != NULL))
        {
        }
      else if (c == '\'' || c == '"')
        {
          /* set terminator */
          p->g1 = (void *) c;

          p->cr1 = X_PP_KV_LAV;
          /* save return state */
          x_parser_push(p, &x_parser_ctx_attr);

          /* set value consumer */
          x_string_clear(&p->r2);
          p->g2 = &p->r2;

          /* copy value using idle function */
          x_parser_set_context(p, &x_parser_ctx_idle);
        }
      else if (x_isalpha(c) || x_isdigit(c) || c == ':' || c == '_' || c < '\0')
        {
          TRACE("LAV\n");
          x_string_putc(&p->r2, c);
          p->cr1 = X_PP_KV_LAV;
        }
      else
        goto ret_to_tag;
    }
  else if (p->cr1 == X_PP_KV_WANT_VAL)
    {
      if (c == '=')
        {
          p->cr1 = X_PP_KV_VAL;
        }
      else if ((x_strchr("\t\r\n ", c) != NULL))
        {
        }
      else if (x_isalpha(c) || c == '_' || c == ':' || c < '\0')
        {
          setattr(p->r1.cbuf, NULL, &p->attrs);
          x_string_clear(&p->r1);
          x_string_clear(&p->r2);
          x_string_putc(&p->r1, c);
          /* clear flags */
          p->cr1 = X_PP_KV_KEY;
        }
      else
        goto ret_to_tag;
    }
  else if (p->cr1 == X_PP_KV_KEY)
    {
      if (x_isalpha(c) || x_isdigit(c) || c == ':' || c == '_' || c < '\0')
        {
          x_string_putc(&p->r1, c);
        }
      else if (c == '=')
        {
          p->cr1 = X_PP_KV_VAL;
        }
      else if ((x_strchr("\t\r\n ", c) != NULL))
        {
          p->cr1 = X_PP_KV_WANT_VAL;
          return 0;
        }
    }
  else
    {
      if (x_isalpha(c) || c == '_' || c == ':' || c < '\0')
        {
          x_string_putc(&p->r1, c);
          p->cr1 = X_PP_KV_KEY;
        }
      else if ((x_strchr("\t\r\n ", c) != NULL))
        {
          return 0;
        }
      else
        goto ret_to_tag;
    }

  return 0;

  ret_to_tag:
    {
      x_string_clear(&p->r1);
      x_string_clear(&p->r2);
      /** @todo This should be restored from stack */
#pragma message(" This should be restored from stack")
      p->cr1 = X_PP_OBJ_OPENING;
      x_parser_set_context(p, &x_parser_ctx_tag);
      p->r_putc(p, c);
    }
  return 0;
}

static int
x_parser_set_context(struct x_push_parser *p, x_putc_ft func)
{
  p->r_putc = func;
  return 0;
}

/**
 *
 */
static void
__x_pparser_init(x_object *o)
{
  struct x_push_parser *p = (struct x_push_parser *) (void *) o;
  ENTER;
  BUG_ON(!o);

  p->cr0 = 0;
  x_string_clear(&p->r1);
  x_string_clear(&p->r2);
  p->bp = x_malloc(X_INI_STACK_SIZE * sizeof(X_INI_STACK_TYPE));
  p->sp = &p->bp[X_INI_STACK_SIZE - 1];
  p->cr0 = 0;
  p->cr1 = X_PP_INI;
  p->g1 = p->g2 = p->g3 = p->g4 = 0;
  attr_list_clear(&p->attrs);
  attr_list_init(&p->attrs);
  p->r_reset = &__x_pparser_init;
  x_parser_set_context(p, &x_parser_ctx_txt);

  EXIT;
}

/* default destructor */
static void
__x_pparser_clean(x_object *o)
{
  ENTER;
  EXIT;
}

/**
 * Xbus class declaration
 */
static struct xobjectclass xmppparser_class;

__x_plugin_visibility
__plugin_init void
xmppparser_init(void)
{
  ENTER;

  x_memset(&xmppparser_class, 0, sizeof(xmppparser_class));
  xmppparser_class.cname = "#xmppparser";
  xmppparser_class.psize = (unsigned int) (sizeof(struct x_push_parser)
      - sizeof(x_object));
  xmppparser_class.on_create = &__x_pparser_init;
  xmppparser_class.on_release = &__x_pparser_clean;
  xmppparser_class.on_assign = &x_object_default_assign_cb;

  x_class_register_ns(xmppparser_class.cname, &xmppparser_class, "gobee");

  EXIT;
  return;
}

PLUGIN_INIT(xmppparser_init);

EXPORT_SYMBOL void
x_parser_init(struct x_push_parser *p)
{
  __x_pparser_init((x_object *) (void *) p);
}

static __inline__ void
x_parser_push(struct x_push_parser *p, void *v)
{
  *p->sp-- = v;
}

static __inline__ void *
x_parser_pop(struct x_push_parser *p)
{
  return *++p->sp;
}

/**
 * Converts xobject to it string representation
 */
static int
__x_parser_object_serialize(x_object *xobj, int
(*cbf)(const char *, size_t, void *), void *cbdata)
{
#define STATE_OPEN 1
#define STATE_CLOSE 2
#define STATE_STOP 3
  int state = STATE_OPEN;
  struct ht_cell *cell;
  x_object *o, *p;

  /* write open tag */
  p = o = xobj;
  do
    {
      switch (state)
        {
      case STATE_OPEN:
        {
          cbf("<", 1, cbdata);
          cbf(x_object_get_name(o),
              x_strlen(x_object_get_name(o)), cbdata);
          for (cell = o->attrs.next; cell; cell = cell->next)
            {
              if (cell->key)
                {
                  cbf(" ", 1, cbdata);
                  cbf(cell->key, x_strlen(cell->key), cbdata);
                  cbf("='", 2, cbdata);
                  if (cell->val)
                    cbf(cell->val, x_strlen(cell->val), cbdata);
                  cbf("'", 1, cbdata);
                }
              else
                {
                  fprintf(stderr, "%s():%d cell->key(%p) && cell->val(%p)\n",
                      __FUNCTION__, __LINE__, cell->key, cell->val);
                }
            }

          if (TREE_NODE(o)->sons || o->content.pos > 0)
            {

              cbf(">", 1, cbdata);

              if (TREE_NODE(o)->sons)
                o = TNODE_TO_XOBJ(TREE_NODE(o)->sons);
              else
                {
                  state = STATE_CLOSE;
                }
            }
          else
            {
              cbf("/>", 2, cbdata);
              if (o == p)
                {
                  state = STATE_STOP;
                }
              else if (x_object_get_sibling(o))
                {
                  o = x_object_get_sibling(o);
                }
              else
                {
                  state = STATE_CLOSE;
                  o = x_object_get_parent(o);
                }
            }
          break;
        }
      case STATE_CLOSE:
        {
          if (o->content.pos > 0)
            {
              cbf(o->content.cbuf, o->content.pos, cbdata);
            }
          cbf("</", 2, cbdata);
          cbf(x_object_get_name(o), x_strlen(x_object_get_name(o)), cbdata);
          cbf(">", 1, cbdata);
          if (o == p)
            state = STATE_STOP;
          else if (x_object_get_sibling(o))
            {
              state = STATE_OPEN;
              o = x_object_get_sibling(o);
            }
          else
            {
              o = x_object_get_parent(o);
            }
          break;
        }
      default:
        state = STATE_STOP;
        break;
        };
    }
  while (state != STATE_STOP);
  return 0;
}

static int
__x_write_fd(const char *str, size_t size, void *cbdata)
{
  return x_write(*(int *) cbdata, str, size);
}

static int
__x_write_xstr(const char *str, size_t size, void *cbdata)
{
  return x_string_write((x_string *) cbdata, str, size);
}

EXPORT_SYMBOL int
x_parser_object_to_string(x_object *xobj, x_string *xstr)
{
  return __x_parser_object_serialize(xobj, &__x_write_xstr, xstr);
}

EXPORT_SYMBOL int
x_parser_object_to_fd(x_object *xobj, int fd)
{
  int _fd = fd;
  return __x_parser_object_serialize(xobj, &__x_write_fd, &_fd);
}

#ifdef XOBJECT_DEBUG
int xobject_testser(void)
  {
    x_object *o;
    x_string xstr = X_STRING_STATIC_INIT;
    o = x_object_new("serialization-test-object");
    x_object_setattr(o,"qqq","qwe");
    x_object_setattr(o,"k1","value1");
    x_object_setattr(o,"k2","value2");
    x_object_setattr(o,"k3","value3");
    x_object_setattr(o,"k4","value4");

    x_parser_object_to_string(o,&xstr);
    printf("Serialization TEST1:\n%s\n",xstr.cbuf);

    x_object_destroy(o);

    return 0;
  }
#endif

