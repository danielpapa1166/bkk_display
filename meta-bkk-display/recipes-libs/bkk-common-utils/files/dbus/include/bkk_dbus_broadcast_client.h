#ifndef BKK_DBUS_BROADCAST_CLIENT_H
#define BKK_DBUS_BROADCAST_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bkk_dbus.h"
#include "bkk_dbus_pub_types.h"
#include "common_types.h"


int init_broadcast_client(const char * bus_name, 
    bkk_dbus_listener_t* clt, bkk_dbus_listener_sig_hdl_t handler, 
    void* user_data, bc_client_t *client);
    
int send_client_request(const char * bus_name, 
  const bc_client_request_t *request, bc_server_data_t * response);



#ifdef __cplusplus
}
#endif

#endif