#ifndef BKK_SCREEN_CLIENT_HPP
#define BKK_SCREEN_CLIENT_HPP

#include "bkk_screen_common_defs.hpp"


bkk_screen_error_code_t bkk_client_acquire_screen_component(
  bkk_screen_component_id_t component_id, int * token); 

bkk_screen_error_code_t bkk_client_release_screen_component(
  int token);

bkk_screen_error_code_t bkk_client_set_info_bar_data(
  const bkk_screen_info_bar_data_t * data);


#endif // BKK_SCREEN_CLIENT_HPP