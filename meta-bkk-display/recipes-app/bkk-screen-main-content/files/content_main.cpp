#include "bkk_screen_client/client.hpp"
#include "bkk_api_client.hpp"
#include <rbuflogd/logger.h>
#include <vector>
#include <string>


int main() {
  rbuflogd_logger_init("ScrCltMc");

  log_info("Main", "Starting BKK Screen Client");

  int key = 0;
  std::string apiKey = "de586d3b-4e9d-4708-a56b-8a46c5ac52a4";

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

  std::vector<std::string> stationIdList = {"F01335", "056234"};

  std::vector<arrival_info_t> arrivals;

  res = api_client::fetch_arrivals(apiKey, stationIdList, arrivals);


  res = bkk_screen_client_set_table_data(key, arrivals);
  if (res != BKK_SCREEN_ERROR_NONE) {
    log_error("Main", (
      "Failed to set table data, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }


  return 0;
}