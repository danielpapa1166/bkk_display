#include "bkk_screen_client/client.hpp"
#include <rbuflogd/logger.h>


int main() {
  rbuflogd_logger_init("ScrCltMc");

  log_info("Main", "Starting BKK Screen Client");

  int key = 0;

  int res = bkk_screen_client_acquire_component(
    BKK_SCREEN_COMPONENT_TABLE, 
    &key);

  if (res != BKK_SCREEN_ERROR_NONE) {
    log_error("Main", (
      "Failed to acquire screen component, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }

  log_info("Main", (
    "Successfully acquired screen component, key: "
    + std::to_string(key)).c_str()
  );

  res = bkk_screen_client_set_table_data(key, 42);
  if (res != BKK_SCREEN_ERROR_NONE) {
    log_error("Main", (
      "Failed to set table data, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }

  return 0;
}