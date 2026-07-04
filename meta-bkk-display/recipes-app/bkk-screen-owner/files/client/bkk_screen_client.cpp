#include "bkk_screen_client/client.hpp"
#include "bkk_screen_client/common_defs.hpp"
#include "bkk_screen_common_priv_defs.hpp"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>


static int uds_open(int * sock_fd) {
  *sock_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (*sock_fd == -1) {
    return -1;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, BKK_SCREEN_UDS_NAME, sizeof(addr.sun_path) - 1);

  const int conn_res = connect(
    *sock_fd, 
    (struct sockaddr *)&addr, 
    sizeof(addr));
  if (conn_res < 0) {
    close(*sock_fd);
    return -1;
  }

  return 0;
}

static bkk_screen_error_code_t uds_send_recv(
    int sock_fd, 
    void * request, 
    size_t request_size,
    void * response, 
    size_t response_size) {
  
  const int send_res = send(
    sock_fd, 
    request, 
    request_size, 
    0);

  if (send_res != request_size) {
    return BKK_SCREEN_ERROR_SOCKET_SEND_FAILED;
  }

  const int recv_res = recv(
    sock_fd, 
    response, 
    response_size, 
    0);

  if (recv_res != response_size) {
    return BKK_SCREEN_ERROR_SOCKET_RECV_FAILED;
  }

  return BKK_SCREEN_ERROR_NONE;
}


bkk_screen_error_code_t bkk_screen_client_acquire_component(
    bkk_screen_component_id_t component_id, int * key) {


  if (key == nullptr) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  if(component_id < 0 || component_id >= BKK_SCREEN_COMPONENT_MAX) {
    return BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
  }

  int sock_fd = -1;
  const int uds_open_res = uds_open(&sock_fd);
  if (uds_open_res != 0) {
    return BKK_SCREEN_ERROR_SOCKET_OPEN_FAILED;
  }

  bkk_screen_uds_message_t request {};
  request.header.cmd_id = BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT;
  request.header.component_id = component_id;


  bkk_screen_uds_message_t response {};
  const bkk_screen_error_code_t uds_res = uds_send_recv(
    sock_fd,
    &request,
    sizeof(request),
    &response,
    sizeof(response));
  close(sock_fd);

  if (uds_res != BKK_SCREEN_ERROR_NONE) {
    return uds_res;
  }

  if(response.header.cmd_id != BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT) {
    return BKK_SCREEN_ERROR_RESPONSE_INVALID;
  }


  if (response.acquire_resp.error_code != BKK_SCREEN_ERROR_NONE) {
    return response.acquire_resp.error_code;
  }


  *key = response.acquire_resp.key;
  return BKK_SCREEN_ERROR_NONE;
}


bkk_screen_error_code_t bkk_screen_client_release_screen_component(int key) {
  (void)key; // Placeholder for future implementation
  return BKK_SCREEN_ERROR_NONE;
}

bkk_screen_error_code_t bkk_screen_client_set_info_bar_data(
    int key, bkk_screen_online_status_t online_status, const char * clock) {

  (void)  key; // Placeholder for future implementation

  if(clock == nullptr) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  int sock_fd = -1;
  const int uds_open_res = uds_open(&sock_fd);
  if (uds_open_res != 0) {
    return BKK_SCREEN_ERROR_SOCKET_OPEN_FAILED;
  }

  bkk_screen_uds_message_t request {};
  request.header.cmd_id = BKK_SCREEN_COMMAND_SET_INFO_BAR_DATA;
  request.header.component_id = BKK_SCREEN_COMPONENT_INFO_BAR;
  request.set_info_bar_data.key = key;
  strncpy(request.set_info_bar_data.clock, clock, BKK_SCREEN_INFO_BAR_CLOCK_MAX_LEN);
  request.set_info_bar_data.online_status = online_status;

  bkk_screen_uds_message_t response {};
  const bkk_screen_error_code_t uds_res = uds_send_recv(
    sock_fd,
    &request,
    sizeof(request),
    &response,
    sizeof(response));
  close(sock_fd);

  if (uds_res != BKK_SCREEN_ERROR_NONE) {
    return uds_res;
  }

  // todo handle response 

  return BKK_SCREEN_ERROR_NONE;
}