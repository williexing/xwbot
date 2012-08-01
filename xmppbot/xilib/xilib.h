#ifndef __XILIB_H__
#define __XILIB_H__

#include <xi.h>

#define XILIB_MAJOR     0
#define XILIB_MINOR     1

typedef struct xilib_comm
{
  int sockfd;
  struct sockaddr_un server_address;
  void *data_out;
  void *data_in;
  int out_size;
  int in_size;
} xilib_comm_t;

int xilib_open(xilib_comm_t *comm);
void xilib_close(xilib_comm_t *comm);
int xilib_test(xilib_comm_t *comm);

static __inline__ int xilib_get_version(void)
{
  return (XILIB_MAJOR<<16 | XILIB_MINOR);
}

/**
 *
 * @defgroup XIAPI Xilib public API
 *
 * @{
 */
#include <xwlib/x_obj.h>
x_object *xi_get_roster_list(void);
x_object *xi_get_roster_entry(const char *jid, x_obj_attr_t *hints);
x_object *xi_del_roster_entry(const char *jid);
x_object *xi_add_roster_entry(const char *jid);
x_object *xi_session_open(const char *jid, int flags, x_obj_attr_t *hints);
x_object *xi_session_close(const char *sid, int flags, x_obj_attr_t *hints);
x_object *xi_session_on_change(int (*cb)(const char *sid, const x_obj_attr_t *params));
x_object *xi_session_on_new(int (*cb)(const char *sid, const x_obj_attr_t *params));
/**
 * @}
 */
#endif /* __XILIB_H__ */
