#ifndef BKK_IPC_UDS_SERVER_H
#define BKK_IPC_UDS_SERVER_H

#include <pthread.h>
#ifdef __cplusplus
extern "C" {
#endif


#include <stddef.h>
#include "bkk_ipc_uds_common_defs.h"


typedef int (*ipc_uds_callback_t)(int client_fd, void * user_data);

typedef struct {
  int sock_fd;
  int event_fd;
  pthread_t thread_fd;
  ipc_uds_callback_t callback;
  void * user_data;
} ipc_uds_server_t;




ipc_uds_err_t ipc_uds_server_init(
  ipc_uds_server_t * server, 
  const char * socket_path, 
  ipc_uds_callback_t callback,
  void * user_data);

ipc_uds_err_t ipc_uds_receive_from_client(int client_fd, 
  void * buffer, size_t buffer_size);
ipc_uds_err_t ipc_uds_send_response(int client_fd, 
  const void * buffer, size_t buffer_size);

void ipc_uds_close_client(int client_fd);

ipc_uds_err_t ipc_uds_cleanup_server(ipc_uds_server_t * server);


#ifdef __cplusplus
}  
#endif

#endif // BKK_IPC_UDS_SERVER_H