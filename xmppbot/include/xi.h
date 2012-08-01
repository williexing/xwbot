#ifndef __XI_H__
#define __XI_H__

#define XI_SRV_PATH          "/tmp/xi_srv"

typedef enum
{
   XI_NONE,
   XI_TEST,
   XI_GET_BUS_CTRL,
   XI_SET_BUS_CTRL,
   XI_GET_CONTACTS,
   XI_GET_CONTACT_INFO,
   XI_SET_CONTACT_INFO,
   XI_GET_SESSIONS,
   XI_GET_SESSION_INFO,
   XI_SET_SESSION_INFO,
   XI_ON_CONTACT_PRESENCE,
   XI_MSG_SESSION_CREATE,
   XI_STD_SESSION_CREATE,
   XI_ON_MSG_SESSION_NEW,
   XI_ON_STD_SESSION_NEW
} xi_op_t;

/*#define XI_WITH_CRC*/
typedef struct xi_hdr
{
  xi_op_t       op;
  int           data_size;
  int           err;
#ifdef XI_WITH_CRC
  int           data_crc;
#endif
} xi_hdr_t;

#endif /* __XI_H__ */
