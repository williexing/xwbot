/*
 * x_utils.h
 *
 *  Created on: 27.10.2010
 *      Author: artur
 */

#ifndef X_UTILS_H_
#define X_UTILS_H_

#include "x_types.h"
#include "x_lib.h"
#include "x_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Time, length, value structure
 */
typedef struct tlv {
	int type;
	int len;
/*	char value[];*/
} tlv_t;

typedef tlv_t *p_tlv_t;

#define container_of(ptr, sample, member)                               \
        (void *)((char *)(ptr)  -                                       \
                 ((char *)&(sample)->member - (char *)(sample)))

typedef struct gobee_img_plane
{
    /**The width of this plane.*/
    int width;
    /**The height of this plane.*/
    int height;
    /**The offset in bytes between successive rows.*/
    int stride;
    /**A pointer to the beginning of the first row.*/
    unsigned char *data;
} gobee_img_plane_t;

typedef gobee_img_plane_t gobee_img_frame_t[3];


/**
 * DOUBLE LINKED LIST API
 * @defgroup LIST_API Linked list API
 *
 * @{ 
 */
EXPORT_SYMBOL void xw_list_init(struct list_entry *l);
EXPORT_SYMBOL void xw_list_insert(struct list_entry *l, struct list_entry *e);
EXPORT_SYMBOL void xw_list_remove(struct list_entry *e);
EXPORT_SYMBOL int xw_list_is_empty(struct list_entry *e);

#define xw_list_for_each(pos, head, member)				\
	for (pos = 0, pos = container_of((head)->next, pos, member);	\
	     &pos->member != (head);					\
	     pos = container_of(pos->member.next, pos, member))

#define xw_list_for_each_safe(pos, tmp, head, member)			\
	for (pos = 0, tmp = 0, 						\
	     pos = container_of((head)->next, pos, member),		\
	     tmp = container_of((pos)->member.next, tmp, member);	\
	     &pos->member != (head);					\
	     pos = tmp,							\
	     tmp = container_of(pos->member.next, tmp, member))

#define xw_list_for_each_reverse(pos, head, member)			\
	for (pos = 0, pos = container_of((head)->prev, pos, member);	\
	     &pos->member != (head);					\
	     pos = container_of(pos->member.prev, pos, member))
/**
 * @}
 */

/**
 * CIRCULAR BUFFER API
 * @defgroup CIRCBUF_API Circular buffer API
 *
 * @{
 */
typedef struct x_circbuf
{
  int rcnt;
  int wcnt;
  uint8_t *start;
  uint8_t *end;
  uint8_t *head;
  uint8_t *tail;
} circular_buffer_t;

EXPORT_SYMBOL void circbuf_init(circular_buffer_t *cb, int size);
EXPORT_SYMBOL void circbuf_deinit(circular_buffer_t *cb);
EXPORT_SYMBOL int circbuf_write(circular_buffer_t *cb, const uint8_t *data, int size);
EXPORT_SYMBOL int circbuf_read(circular_buffer_t *cb, uint8_t *data, int size);
EXPORT_SYMBOL int circbuf_length(circular_buffer_t *cb);
/**
 * @}
 */
  
/**
 * HASH TABLE API
 *
 * @defgroup HT_API Hash table API
 * @{
 */

#define EQ(s1, s2)   (x_strcasecmp(s1, s2) == 0)
#if 0
#define EQN(s1, s2)   \
  ((strcspn(s1,"\t\r\n\ \0") == strcspn(s2,"\t\r\n\ \0"))\
    &&(x_strncasecmp(s1, s2, strcspn(s1,"\n\r")) == 0))
#else
#define EQN(s1, s2)   \
    (1&&(x_strncasecmp(s1, s2, strcspn(s1,"\n\r")) == 0))
#endif
#define NEQ(s1, s2)   (x_strcasecmp(s1, s2) != 0)
#define EQKEY(s1, s2)   (x_strcmp(s1, s2) == 0)
#define FREEKEY(s)      do { x_free(s); s = NULL; } while (0)
#define FREEVAL(s)      do { x_free(s); s = NULL; } while (0)
#define SETVAL(at,s)    (at = x_strdup(s))
#define SETKEY(at,s)    (at = x_strdup(s))
#define KEYFMT          "key:   %s"
#define VALFMT          "value: %s"

struct ht_cell **ht_create_table(void);
int ht_hash(KEY key);
struct ht_cell *ht_get_cell(KEY key, struct ht_cell **hashtable, int *indx);
EXPORT_SYMBOL VAL ht_get(KEY key, struct ht_cell **hashtable, int *indx);
EXPORT_SYMBOL VAL ht_lowcase_get(KEY key, struct ht_cell **hashtable, int *indx);
EXPORT_SYMBOL void ht_set(KEY key, VAL val, struct ht_cell **hashtable);
int ht_del(KEY key, struct ht_cell **hashtable);
struct ht_cell *ht_next(struct ht_celliter *ci, struct ht_cell **hashtable);
struct ht_cell *ht_nullifyiter(struct ht_celliter *ci, struct ht_cell **hashtable);

static __inline__ void
str_swap_chars(char *str, char c1, char c2)
{
  while (*str)
    {
      if (*str == c1)
        *str = c2;
      str++;
    }
}
/**
 * @}
 */

#ifdef HTABLE_DEBUG
void ht_printcell(struct ht_cell *ptr);
void ht_printtable(struct ht_cell **hashtable);
void ht_testtable(void);
#else
#define ht_printcell(ptr)	(0)
#define ht_printtable(tbl)	(0)
#define ht_testtable()		(0)
#endif

/**
 * ATRRIBUTE LIST API
 * @defgroup Attribute API
 *
 * @{ 
 */
EXPORT_SYMBOL void attr_list_clear(x_obj_attr_t *head);
EXPORT_SYMBOL void attr_list_init(x_obj_attr_t *head);
EXPORT_SYMBOL int setattr(KEY k, VAL v, x_obj_attr_t *head);
EXPORT_SYMBOL int delattr(KEY k, x_obj_attr_t *head);
EXPORT_SYMBOL const x_string_t getattr(KEY key, x_obj_attr_t *head);
EXPORT_SYMBOL int attr_list_is_empty(x_obj_attr_t *head);
/**
 * @}
 */
void print_attr_list(x_obj_attr_t *head);

/**
 * NETWORKING UTILS
 * @defgroup Networking helper utilities
 *
 * @{
 */
#ifndef WIN32
#include <stddef.h>
#include <sys/socket.h>

struct sockaddr **icectl_get_local_ips(void);
struct sockaddr **__x_bus_get_local_addr_list(void);
/**
 * @}
 */
#endif

EXPORT_SYMBOL struct sockaddr **__x_bus_get_local_addr_list2(void);
EXPORT_SYMBOL int set_sock_to_non_blocking (int s);
EXPORT_SYMBOL int unset_sock_to_non_blocking (int s);

EXPORT_SYMBOL int x_thread_run(THREAD_T *,t_func, void *);


#define X_PRIV_STR_MODE "$mode"
#define X_PRIV_STR_WIDTH "width"

#ifdef __cplusplus
}
#endif

#endif /* X_UTILS_H_ */
