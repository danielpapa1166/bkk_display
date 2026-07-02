#include "bkk_screen_client.hpp"
#include "bkk_screen_common_defs.hpp"
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

static int uds_send_recv(
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
    return -1;
  }

  const int recv_res = recv(
    sock_fd, 
    response, 
    response_size, 
    0);

  if (recv_res != response_size) {
    return -1;
  }

  return 0;
}


bkk_screen_error_code_t bkk_client_acquire_screen_component(
    bkk_screen_component_id_t component_id, int * token) {


  if (token == nullptr) {
    return BKK_SCREEN_ERROR_OTHER;
  }

  if(component_id < 0 || component_id >= BKK_SCREEN_COMPONENT_MAX) {
    return BKK_SCREEN_ERROR_OTHER;
  }

  int sock_fd = -1;
  if (uds_open(&sock_fd) != 0) {
    return BKK_SCREEN_ERROR_OTHER;
  }

  bkk_screen_uds_request_t request {};
  request.cmd_id = BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT;
  bkk_screen_acquire_component_request_t * acquire_req = 
    reinterpret_cast<bkk_screen_acquire_component_request_t *>(request.payload);
  acquire_req->component_id = component_id;

  bkk_screen_uds_response_t response {};
  const int uds_res = uds_send_recv(
    sock_fd,
    &request,
    sizeof(request),
    &response,
    sizeof(response));
  close(sock_fd);

  if (uds_res != 0) {
    return BKK_SCREEN_ERROR_OTHER;
  }

  bkk_screen_acquire_component_response_t * acquire_resp =
    reinterpret_cast<bkk_screen_acquire_component_response_t *>(response.payload);
  if (acquire_resp->error_code != BKK_SCREEN_ERROR_NONE) {
    return acquire_resp->error_code;
  }

  *token = acquire_resp->component.token;
  return BKK_SCREEN_ERROR_NONE;

}


bkk_screen_error_code_t bkk_client_release_screen_component(int token) {
  (void)token; // Placeholder for future implementation
  return BKK_SCREEN_ERROR_NONE;
}

bkk_screen_error_code_t bkk_client_set_info_bar_data(
    const bkk_screen_info_bar_data_t * data) {

  if(data == nullptr) {
    return BKK_SCREEN_ERROR_OTHER;
  }

  int sock_fd = -1;
  if (uds_open(&sock_fd) != 0) {
    return BKK_SCREEN_ERROR_OTHER;
  }

  bkk_screen_uds_request_t request {};
  request.cmd_id = BKK_SCREEN_COMMAND_SET_INFO_BAR_DATA;
  bkk_screen_info_bar_data_t * info_bar_data =
    reinterpret_cast<bkk_screen_info_bar_data_t *>(request.payload);
  *info_bar_data = *data;

  bkk_screen_uds_response_t response {};
  const int uds_res = uds_send_recv(
    sock_fd,
    &request,
    sizeof(request),
    &response,
    sizeof(response));
  close(sock_fd);

  if (uds_res != 0) {
    return BKK_SCREEN_ERROR_OTHER;
  }

  return BKK_SCREEN_ERROR_NONE;
}