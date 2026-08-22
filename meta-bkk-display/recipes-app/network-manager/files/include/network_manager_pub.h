#ifndef NETWORK_MANAGER_PUB_H
#define NETWORK_MANAGER_PUB_H

#ifdef __cplusplus
extern "C" {
#endif 

#include <bkk_utils/bkk_dbus_pub_types.h>


#define NETWORK_MANAGER_DBUS_NAME                 "NetworkManager"

typedef enum {
  NETWORK_MANAGER_MODE_ACCESS_POINT,
  NETWORK_MANAGER_MODE_WIFI_CLIENT,
  NETWORK_MANAGER_MODE_UNKNOWN
} network_manager_mode_t;


typedef struct {
  network_manager_mode_t mode;
  // other fields to be defined 
} network_manager_data_t;


typedef union {
  network_manager_data_t network_manager_data;
  bc_server_data_t bc_server_data;
} bc_data_un;


#define BKK_DISPLAY_ACCESS_POINT_NAME       "BKK-Display-Setup"
#define BKK_DISPLAY_ACCESS_POINT_IP         "192.168.4.1"   
#define BKK_DISPLAY_CONFIG_SERVER_PORT      8080 


#ifdef __cplusplus
}
#endif


#endif