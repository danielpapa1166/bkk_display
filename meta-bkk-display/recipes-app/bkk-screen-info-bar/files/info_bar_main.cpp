#include <cstdio>
#include "bkk_screen_client/client.hpp"
#include "clock_update.hpp"
#include "online_check.hpp"
#include <string>
#include <unistd.h>
#include <rbuflogd/logger.h>

int main() {

  printf("Starting BKK Screen Client\n");

  rbuflogd_logger_init("ScrCltIb");

  online_check::online_check_init();
  int key = 0;
  int res = bkk_screen_client_acquire_component(
    BKK_SCREEN_COMPONENT_INFO_BAR, 
    &key);

  if (res != BKK_SCREEN_ERROR_NONE) {
    printf("Failed to acquire screen component, error code: %d\n", res);
    return 1;
  }

  printf("Successfully acquired screen component, key: %d\n", key);
  log_info("Main", (
    "Successfully acquired screen component, key: " + std::to_string(key)).c_str());

  while(1) {
    std::string current_time;
    res = screen_clock_update::get_current_time_CET(current_time);
    if (res != 0) {
      log_error("Main", "Failed to get current time");
      return 1;
    }

    bkk_screen_info_bar_data_t info_bar_data {};
    snprintf(
      info_bar_data.clock, 
      sizeof(info_bar_data.clock), 
      "%s",
      current_time.c_str());
  
    info_bar_data.online_status = online_check::is_online() 
      ? BKK_SCREEN_ONLINE_STATUS_ONLINE 
      : BKK_SCREEN_ONLINE_STATUS_OFFLINE;


    res = bkk_screen_client_set_info_bar_data(key, &info_bar_data);
    if (res != BKK_SCREEN_ERROR_NONE) {
      log_error("Main", 
        ("Failed to set info bar data, error code: " 
          + std::to_string(res)).c_str());
      return 1;
    }

    sleep(1);
  }

  return 0;
}