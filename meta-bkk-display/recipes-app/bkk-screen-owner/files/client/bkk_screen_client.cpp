#include "bkk_screen_client/client.hpp"
#include "bkk_screen_client/common_defs.hpp"
#include "bkk_screen_common_priv_defs.hpp"
#include <bkk_utils/bkk_ipc_uds_client.h>
#include <bkk_utils/bkk_ipc_uds_common_defs.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>


bkk_screen_error_code_t bkk_screen_client_acquire_component(
    bkk_screen_component_id_t component_id, int * key) {


  if (key == nullptr) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  if(component_id < 0 || component_id >= BKK_SCREEN_COMPONENT_MAX) {
    return BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND;
  }

  bkk_screen_uds_message_t request {};
  request.header.cmd_id = BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT;
  request.header.component_id = component_id;
  bkk_screen_uds_message_t response {};

  const ipc_uds_err_t ipc_uds_res = ipc_uds_client_send_recv(
    BKK_SCREEN_UDS_NAME,
    &request,
    sizeof(request),
    &response,
    sizeof(response));


  if (ipc_uds_res != IPC_UDS_ERR_NONE) {
    return BKK_SCREEN_ERROR_SOCKET_OPEN_FAILED;
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

bkk_screen_error_code_t bkk_screen_client_ping(int key, bkk_screen_component_id_t component_id) {
  bkk_screen_uds_message_t request {};
  request.ping.header.cmd_id = BKK_SCREEN_COMMAND_PING;
  request.ping.header.component_id = component_id;

  bkk_screen_uds_message_t response {};

  const ipc_uds_err_t ipc_uds_res = ipc_uds_client_send_recv(
    BKK_SCREEN_UDS_NAME,
    &request,
    sizeof(request),
    &response,
    sizeof(response));


  if (ipc_uds_res != IPC_UDS_ERR_NONE) {
    return BKK_SCREEN_ERROR_SOCKET_OPEN_FAILED;
  }

  if(response.header.cmd_id != BKK_SCREEN_COMMAND_PING) {
    return BKK_SCREEN_ERROR_RESPONSE_INVALID;
  }

  return response.generic_resp.error_code;
}


bkk_screen_error_code_t bkk_screen_client_release_screen_component(
    int key, bkk_screen_component_id_t component_id) {

  bkk_screen_uds_message_t request {};
  request.header.cmd_id = BKK_SCREEN_COMMAND_RELEASE_COMPONENT;
  request.header.component_id = component_id;
  bkk_screen_uds_message_t response {};

  const ipc_uds_err_t ipc_uds_res = ipc_uds_client_send_recv(
    BKK_SCREEN_UDS_NAME,
    &request,
    sizeof(request),
    &response,
    sizeof(response));


  if (ipc_uds_res != IPC_UDS_ERR_NONE) {
    return BKK_SCREEN_ERROR_SOCKET_OPEN_FAILED;
  }


  return response.generic_resp.error_code;
}

bkk_screen_error_code_t bkk_screen_client_set_info_bar_data(
    int key, 
    bkk_screen_online_status_t online_status, 
    const char * clock, 
    const char * ip_address) {
  
  if(clock == nullptr) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  (void)  key; // Placeholder for future implementation
  bkk_screen_uds_message_t request {};
  request.header.cmd_id = BKK_SCREEN_COMMAND_SET_DATA;
  request.header.component_id = BKK_SCREEN_COMPONENT_INFO_BAR;
  request.set_info_bar_data.key = key;
  strncpy(request.set_info_bar_data.clock, 
    clock, BKK_SCREEN_INFO_BAR_CLOCK_MAX_LEN);
  strncpy(request.set_info_bar_data.ip_address,
    ip_address, BKK_SCREEN_IP_LEN);

  request.set_info_bar_data.online_status = online_status;
  bkk_screen_uds_message_t response {};

  const ipc_uds_err_t ipc_uds_res = ipc_uds_client_send_recv(
    BKK_SCREEN_UDS_NAME,
    &request,
    sizeof(request),
    &response,
    sizeof(response));


  if (ipc_uds_res != IPC_UDS_ERR_NONE) {
    return BKK_SCREEN_ERROR_SOCKET_OPEN_FAILED;
  }


  // todo handle response 

  return BKK_SCREEN_ERROR_NONE;
}

bkk_screen_error_code_t bkk_screen_client_set_status_screen_data(
    int key, const char * status_text, size_t status_text_len) {

  if(status_text == nullptr || status_text_len == 0) {
    return BKK_SCREEN_ERROR_INVALID_PARAM;
  }

  bkk_screen_uds_message_t request {};
  request.header.cmd_id = BKK_SCREEN_COMMAND_SET_DATA;
  request.header.component_id = BKK_SCREEN_COMPONENT_STATUS_SCREEN;
  request.set_status_screen_data.key = key;

  // Ensure we don't exceed the maximum length of the status text
  size_t copy_len = std::min(status_text_len, sizeof(request.set_status_screen_data.status_text) - 1);
  strncpy(request.set_status_screen_data.status_text, status_text, copy_len);
  request.set_status_screen_data.status_text[copy_len] = '\0'; // Null-terminate

  bkk_screen_uds_message_t response {};

  const ipc_uds_err_t ipc_uds_res = ipc_uds_client_send_recv(
    BKK_SCREEN_UDS_NAME,
    &request,
    sizeof(request),
    &response,
    sizeof(response));


  if (ipc_uds_res != IPC_UDS_ERR_NONE) {
    return BKK_SCREEN_ERROR_SOCKET_OPEN_FAILED;
  }

  // todo handle response 

  return BKK_SCREEN_ERROR_NONE;
}


bkk_screen_error_code_t bkk_screen_client_set_table_data(
    int key, std::vector<arrival_info_t>& arrivals) {

  bkk_screen_uds_message_t request {};
  request.header.cmd_id = BKK_SCREEN_COMMAND_SET_DATA;
  request.header.component_id = BKK_SCREEN_COMPONENT_TABLE;

  request.set_table_data.key = key;
  request.set_table_data.num_arrivals 
    = arrivals.size() > BKK_SCREEN_MAX_ARRIVALS 
      ? BKK_SCREEN_MAX_ARRIVALS 
      : arrivals.size();
      
  for (size_t i = 0; i < arrivals.size() && i < BKK_SCREEN_MAX_ARRIVALS; i++) {
    request.set_table_data.arrivals[i] = arrivals[i];
  }

  bkk_screen_uds_message_t response {};

  const ipc_uds_err_t ipc_uds_res = ipc_uds_client_send_recv(
    BKK_SCREEN_UDS_NAME,
    &request,
    sizeof(request),
    &response,
    sizeof(response));


  if (ipc_uds_res != IPC_UDS_ERR_NONE) {
    return BKK_SCREEN_ERROR_SOCKET_OPEN_FAILED;
  }


  return BKK_SCREEN_ERROR_NONE;
}
