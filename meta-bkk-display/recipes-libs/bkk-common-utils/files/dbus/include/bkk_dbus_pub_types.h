#ifndef BKK_DBUS_PUB_TYPES_H
#define BKK_DBUS_PUB_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#define PAYLOAD_SIZE                        256

typedef enum {
  bkk_dbus_err_none = 0,
  bkk_dbus_err_other = 1,
  // list to be extendeed
} bkk_dbus_err_t; 


typedef struct {
  char server_data[PAYLOAD_SIZE];
} bc_server_data_t;

typedef struct {
  char request[PAYLOAD_SIZE];
} bc_client_request_t;


#ifdef __cplusplus
}
#endif


#endif
