#include <cstdio>
#include "bkk_screen_client/client.hpp"

int main() {

  printf("Starting BKK Screen Client\n");
  int token = 0;
  int res = bkk_client_acquire_screen_component(
    BKK_SCREEN_COMPONENT_INFO_BAR, 
    &token);

  if (res != BKK_SCREEN_ERROR_NONE) {
    printf("Failed to acquire screen component, error code: %d\n", res);
    return 1;
  }

  printf("Successfully acquired screen component, token: %d\n", token);

  bkk_screen_info_bar_data_t info_bar_data {};
  info_bar_data.clock = "12:34 PM";


  res = bkk_client_set_info_bar_data(&info_bar_data);
  if (res != BKK_SCREEN_ERROR_NONE) {
    printf("Failed to set info bar data, error code: %d\n", res);
    return 1;
  }
  printf("Successfully set info bar data\n");


  return 0;

}