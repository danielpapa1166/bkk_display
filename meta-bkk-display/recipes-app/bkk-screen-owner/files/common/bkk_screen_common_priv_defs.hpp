#ifndef BKK_SCREEN_COMMON_PRIV_DEFS_HPP
#define BKK_SCREEN_COMMON_PRIV_DEFS_HPP

#include "bkk_screen_client/common_defs.hpp"
#include <stdint.h>
#include <string>

#define BKK_SCREEN_UDS_NAME                   "/tmp/bkk_screen.sock"
#define BKK_SCREEN_INFO_BAR_CLOCK_MAX_LEN     6
#define BKK_SCREEN_UDS_PAYLOAD_MAX_SIZE       1024
#define BKK_SCREEN_IP_LEN                     16

#define BKK_SCREEN_STATUS_TEXT_MAX_LEN        128

typedef enum {
  BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT = 0, 
  BKK_SCREEN_COMMAND_PING,
  BKK_SCREEN_COMMAND_RELEASE_COMPONENT,
  BKK_SCREEN_COMMAND_SET_DATA,
  BKK_SCREEN_COMMAND_MAX
} bkk_screen_command_id_t;


typedef struct {
  bkk_screen_command_id_t cmd_id;
  bkk_screen_component_id_t component_id;
} msg_header_t;

typedef struct {
  msg_header_t header;
} bkk_screen_acq_comp_req_t;

typedef struct {
  msg_header_t header;
  bkk_screen_error_code_t error_code;
  int key; 
} bkk_screen_acq_comp_resp_t;


typedef struct {
  msg_header_t header;
} bkk_screen_ping_req_t;

typedef struct {
  msg_header_t header;
  int key; 
  char clock[BKK_SCREEN_INFO_BAR_CLOCK_MAX_LEN];
  bkk_screen_online_status_t online_status;
  char ip_address[BKK_SCREEN_IP_LEN];
} bkk_screen_set_info_bar_data_t;


typedef struct {
  msg_header_t header;
  int key; 
  char status_text[BKK_SCREEN_STATUS_TEXT_MAX_LEN];
} bkk_screen_set_status_screen_data_t;


typedef struct {
  msg_header_t header;
  int key; 
  helper_screen_data_t helper_data;
} bkk_screen_set_helper_screen_data_t;

typedef struct {
  msg_header_t header;
  int key; 
  int num_arrivals;
  arrival_info_t arrivals[BKK_SCREEN_MAX_ARRIVALS];
} bkk_screen_set_table_data_t;

typedef struct {
  msg_header_t header;
  bkk_screen_error_code_t error_code;
} bkk_screen_generic_resp_t;


typedef struct {
  uint8_t payload[BKK_SCREEN_UDS_PAYLOAD_MAX_SIZE];
} uds_trx_buffer_t;



typedef union {
  msg_header_t header;
  bkk_screen_ping_req_t ping; 
  uds_trx_buffer_t buffer; // general buffer for all requests and responses

  bkk_screen_acq_comp_req_t acquire_req;
  bkk_screen_acq_comp_resp_t acquire_resp;
  bkk_screen_set_info_bar_data_t set_info_bar_data;
  bkk_screen_set_status_screen_data_t set_status_screen_data;
  bkk_screen_set_helper_screen_data_t set_helper_screen_data;
  bkk_screen_set_table_data_t set_table_data;
  bkk_screen_generic_resp_t generic_resp;

} bkk_screen_uds_message_t;

#endif // BKK_SCREEN_COMMON_PRIV_DEFS_HPP