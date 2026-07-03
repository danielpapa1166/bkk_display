#ifndef ONLINE_CHECK_HPP
#define ONLINE_CHECK_HPP
#include <cstdint>


namespace online_check {

typedef enum {
  online_check_error_none = 0,
} online_check_error_t; 

typedef struct {
  uint64_t last_response_time_ms;
  bool is_online;
  online_check_error_t error_code;
} online_status_t; 

void online_check_init();
bool is_online();

}

#endif // ONLINE_CHECK_HPP