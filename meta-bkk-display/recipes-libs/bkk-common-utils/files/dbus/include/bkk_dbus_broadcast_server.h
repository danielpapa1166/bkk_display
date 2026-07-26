#ifndef BKK_DBUS_BROADCAST_SERVER_H
#define BKK_DBUS_BROADCAST_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bkk_dbus.h"
#include "bkk_dbus_pub_types.h"
#include "common_types.h"


int init_broadcast_server(const char * bus_name, 
  bkk_dbus_listener_t* clt, bkk_dbus_listener_sig_hdl_t handler, 
  void* user_data, bc_server_t *server);

int serve_data(bc_server_t *server, bc_server_data_t *server_data);


#ifdef __cplusplus
}
#endif

#endif