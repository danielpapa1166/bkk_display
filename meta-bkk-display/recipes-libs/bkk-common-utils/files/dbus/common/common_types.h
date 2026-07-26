#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bkk_dbus.h"
#include "bkk_dbus_pub_types.h"


#define BROADCAST_BUS_NAME_TEMPLATE         "BKK.broadcast.%s"


typedef enum {
  BC_MSG_TYPE_SERVER_DATA = 0,
  BC_MSG_TYPE_CLIENT_REQUEST = 1,
} broadcast_message_type_t;

typedef struct {
  bkk_dbus_listener_sig_hdl_t client_request_handler;
  void* user_data;
  const char* bus_name;
} bc_server_t;

typedef struct {
  bkk_dbus_listener_sig_hdl_t server_response_handler;
  void* user_data;
  const char* bus_name;
} bc_client_t;



typedef struct {
  broadcast_message_type_t msg_type;
  union {
    bc_server_data_t server_data;
    bc_client_request_t client_request;
  } data;
} broadcast_message_t; 




#ifdef __cplusplus
}
#endif

#endif