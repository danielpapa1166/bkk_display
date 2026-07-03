#ifndef BKK_SCREEN_COMMON_DEFS_HPP
#define BKK_SCREEN_COMMON_DEFS_HPP

#include <string>
#include <stdbool.h>

typedef enum {
  BKK_SCREEN_ERROR_NONE, 
  BKK_SCREEN_ERROR_INVALID_PARAM,
  BKK_SCREEN_ERROR_COMPONENT_NOT_FOUND,
  BKK_SCREEN_ERROR_SOCKET_OPEN_FAILED,
  BKK_SCREEN_ERROR_SOCKET_SEND_FAILED,
  BKK_SCREEN_ERROR_SOCKET_RECV_FAILED,
  BKK_SCREEN_ERROR_RESPONSE_INVALID,
  BKK_SCREEN_ERROR_OTHER 
} bkk_screen_error_code_t;

typedef enum {
  BKK_SCREEN_COMPONENT_INFO_BAR = 0, 
  BKK_SCREEN_COMPONENT_MAX
} bkk_screen_component_id_t;


typedef struct {
  std::string clock; 
} bkk_screen_info_bar_data_t;



#endif // BKK_SCREEN_COMMON_DEFS_HPP