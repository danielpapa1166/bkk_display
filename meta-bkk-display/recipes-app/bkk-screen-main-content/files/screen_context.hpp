#ifndef SCREEN_CONTEXT_HPP
#define SCREEN_CONTEXT_HPP

#include "bkk_api_client.hpp"
#include <string>
#include <vector>

typedef enum context_state{
  UNINITIALIZED = 0,
  REPORT_STATUS, 
  DISPLAY_ARRIVAL,
  RELEASE_COMPONENT
} main_context_state_t;


int init_screen_context(); 
int send_screen_ping();
int switch_context(main_context_state_t new_state);
int put_screen_text(const std::string & text);
int send_arrival_info(std::vector<arrival_info_t> & arrivals);

#endif // SCREEN_CONTEXT_HPP