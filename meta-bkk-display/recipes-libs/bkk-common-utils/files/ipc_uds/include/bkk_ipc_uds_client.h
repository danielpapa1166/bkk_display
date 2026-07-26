#ifndef BKK_IPC_UDS_CLIENT_H
#define BKK_IPC_UDS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stddef.h>
#include "bkk_ipc_uds_common_defs.h"


ipc_uds_err_t ipc_uds_client_send_recv(
  const char * socket_path, 
  void * request, 
  size_t request_size,
  void * response, 
  size_t response_size);




#ifdef __cplusplus
}  
#endif

#endif // BKK_IPC_UDS_CLIENT_H