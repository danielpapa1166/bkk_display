#ifndef CONFIG_SERVER_PUB_H
#define CONFIG_SERVER_PUB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <bkk_utils/bkk_dbus_pub_types.h>

#define CONFIG_SERVER_DBUS_NAME                 "ConfigServer"

typedef enum {
  CONFIG_SERVER_NEW_DATA_AVAILABLE = 0xA0,
  CONFIG_SERVER_DONT_CARE = 0x20,
  CONFIG_SERVER_UNKNOWN_SIGNAL = 0xFF
} config_server_signal_t;


typedef struct {
  config_server_signal_t signal;
  // other fields to be defined 
} config_server_data_t;

typedef union {
  config_server_data_t config_server_data;
  bc_server_data_t bc_server_data;
} bc_config_server_un;


#ifdef __cplusplus
}
#endif

#endif 