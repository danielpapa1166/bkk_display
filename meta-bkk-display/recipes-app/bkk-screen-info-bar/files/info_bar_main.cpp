#include <cstdio>
#include "bkk_screen_client/client.hpp"
#include "clock_update.hpp"
#include <string>

int main() {

  printf("Starting BKK Screen Client\n");
  int key = 0;
  int res = bkk_screen_client_acquire_component(
    BKK_SCREEN_COMPONENT_INFO_BAR, 
    &key);

  if (res != BKK_SCREEN_ERROR_NONE) {
    printf("Failed to acquire screen component, error code: %d\n", res);
    return 1;
  }

  printf("Successfully acquired screen component, key: %d\n", key);

  std::string current_time;
  res = screen_clock_update::get_current_time_CET(current_time);
  if (res != 0) {
    printf("Failed to get current time, error code: %d\n", res);
    return 1;
  }
  printf("Current time in CET: %s\n", current_time.c_str());
  bkk_screen_info_bar_data_t info_bar_data {};
  snprintf(
    info_bar_data.clock, 
    sizeof(info_bar_data.clock), 
    "%s",
    current_time.c_str());
  
  info_bar_data.online_status = BKK_SCREEN_ONLINE_STATUS_ONLINE;


  res = bkk_screen_client_set_info_bar_data(key, &info_bar_data);
  if (res != BKK_SCREEN_ERROR_NONE) {
    printf("Failed to set info bar data, error code: %d\n", res);
    return 1;
  }
  printf("Successfully set info bar data\n");


  return 0;

}