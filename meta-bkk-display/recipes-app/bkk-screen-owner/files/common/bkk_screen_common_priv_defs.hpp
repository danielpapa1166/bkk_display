#ifndef BKK_SCREEN_COMMON_PRIV_DEFS_HPP
#define BKK_SCREEN_COMMON_PRIV_DEFS_HPP

#include "bkk_screen_client/common_defs.hpp"
#include <stdint.h>
#include <string>

#define BKK_SCREEN_UDS_NAME               "/tmp/bkk_screen.sock"
#define BKK_SCREEN_UDS_PAYLOAD_MAX_SIZE   256

typedef enum {
  BKK_SCREEN_INTERNAL_UDS_ERR_NONE = 0,
  BKK_SCREEN_INTERNAL_UDS_ERR_INVALID_PARAM,
  BKK_SCREEN_INTERNAL_UDS_ERR_COMP_TAKEN,
  
} bkk_screen_internal_uds_err_t; 

typedef enum {
  BKK_SCREEN_COMMAND_ACQUIRE_COMPONENT = 0, 
  BKK_SCREEN_COMMAND_RELEASE_COMPONENT,
  BKK_SCREEN_COMMAND_SET_INFO_BAR_DATA,
  BKK_SCREEN_COMMAND_MAX
} bkk_screen_command_id_t;


typedef struct {
  bkk_screen_component_id_t component_id;
} bkk_screen_acquire_component_request_t;

typedef struct {
  bkk_screen_error_code_t error_code;
  bkk_screen_component_id_t component_id;
  int key; 
} bkk_screen_acquire_component_response_t;


typedef struct {
  bkk_screen_command_id_t cmd_id; 
  uint8_t payload[BKK_SCREEN_UDS_PAYLOAD_MAX_SIZE];
} bkk_screen_uds_request_t;

typedef struct {
  bkk_screen_command_id_t cmd_id; 
  uint8_t payload[BKK_SCREEN_UDS_PAYLOAD_MAX_SIZE];
} bkk_screen_uds_response_t;

#endif // BKK_SCREEN_COMMON_PRIV_DEFS_HPP