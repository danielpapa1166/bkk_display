#ifndef BKK_SCREEN_CLIENT_HPP
#define BKK_SCREEN_CLIENT_HPP
#include <stddef.h>
#include <vector>
#include "bkk_screen_client/common_defs.hpp"


bkk_screen_error_code_t bkk_screen_client_acquire_component(
  bkk_screen_component_id_t component_id, int * key); 

bkk_screen_error_code_t bkk_screen_client_ping(
  int key, bkk_screen_component_id_t component_id);

bkk_screen_error_code_t bkk_screen_client_release_screen_component(
  int key, bkk_screen_component_id_t component_id);

bkk_screen_error_code_t bkk_screen_client_set_info_bar_data(
  int key, bkk_screen_online_status_t online_status, const char * clock);

bkk_screen_error_code_t bkk_screen_client_set_status_screen_data(
  int key, const char * status_text, size_t status_text_len);

bkk_screen_error_code_t bkk_screen_client_set_table_data(
  int key, std::vector<arrival_info_t>& arrivals);

#endif // BKK_SCREEN_CLIENT_HPP