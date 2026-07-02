#include "bkk_screen_client.hpp"
#include "bkk_screen_common_defs.hpp"
#include "bkk_screen_common_priv_defs.hpp"

bkk_screen_error_code_t bkk_screen_take_info_bar_control(
    bkk_screen_component_t * component) {
  
  int shmem_fd = shm_open(BKK_SCREEN_SHMEM_NAME, O_RDWR, 0);

  


  return BKK_SCREEN_ERROR_NONE;
}