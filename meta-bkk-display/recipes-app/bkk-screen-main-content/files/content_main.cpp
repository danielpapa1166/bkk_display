#include "bkk_screen_client/client.hpp"
#include "bkk_api_client.hpp"
#include <bkk_screen_client/common_defs.hpp>
#include <ctime>
#include <rbuflogd/logger.h>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/select.h>


static int setup_timer(int time_sec, int * timer_fd, fd_set * readfds) {
  *timer_fd = timerfd_create(CLOCK_REALTIME, 0);
  if (*timer_fd < 0) {
    return -1;
  }

  struct itimerspec timerSpec;
  timerSpec.it_interval.tv_sec = time_sec; // Interval for periodic timer
  timerSpec.it_interval.tv_nsec = 0;
  timerSpec.it_value.tv_sec = time_sec; // Initial expiration
  timerSpec.it_value.tv_nsec = 0;

  const int res = timerfd_settime(
    *timer_fd, 
    0, 
    &timerSpec, 
    nullptr);

  if (res < 0) {
    return -1;
  }
  
  FD_ZERO(readfds);
  FD_SET(*timer_fd, readfds);
  return 0;
}

static int wait_on_timer(int timer_fd, fd_set * readfds) {
  int res = select(
    timer_fd + 1, 
    readfds, 
    nullptr, 
    nullptr, 
    nullptr);

  if (res < 0) {
    return -1;
  }

  char buf[8];
  ssize_t n = read(timer_fd, buf, sizeof(buf));
  if (n < 0) {
    return -1;
  }

  return 0;
}


int main() {
  rbuflogd_logger_init("ScrCltMc");

  log_info("Main", "Starting BKK Screen Client");

  int res = 0;
  int key = 0;
  
  res = bkk_screen_client_acquire_component(
    BKK_SCREEN_COMPONENT_STATUS_SCREEN, 
    &key
  );

  sleep(1); 

  const char * status_text = "Hello from Client!";
  res = bkk_screen_client_set_status_screen_data(
    key, status_text, strlen(status_text)
  );

  sleep(1);


  std::string apiKey;
  res = api_client::load_api_key(apiKey);

  if (res != 0) {
    log_error("Main", (
      "Failed to load API key, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }


  std::vector<std::string> stationIdList;
  std::vector<std::string> stationNameList;

  res = api_client::load_station_ids(
    stationIdList, 
    stationNameList
  );

  if (res != 0) {
    log_error("Main", (
      "Failed to load station IDs, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }

  bkk_screen_client_release_screen_component(
    key, BKK_SCREEN_COMPONENT_STATUS_SCREEN); 

  res = bkk_screen_client_acquire_component(
    BKK_SCREEN_COMPONENT_TABLE, 
    &key);

  if (res != BKK_SCREEN_ERROR_NONE) {
    log_error("Main", (
      "Failed to acquire screen component, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }

  int tfd;
  fd_set readfds;
  res = setup_timer(1, &tfd, &readfds);
  if (res < 0) {
    log_error("Main", "Failed to setup timer");
    return 1;
  }


  log_info("Main", (
    "Successfully acquired screen component, key: "
    + std::to_string(key)).c_str()
  );

  static int timer_cnt = 0; 
  while(1) {
    res = wait_on_timer(tfd, &readfds);

    if (res < 0) {
      log_error("Main", "Select error on timerfd");
      return 1;
    }
    
    bkk_screen_error_code_t ping_res = bkk_screen_client_ping(
      key, BKK_SCREEN_COMPONENT_TABLE);

    if(ping_res != BKK_SCREEN_ERROR_NONE) {
      log_error("Main", (
        "Failed to ping screen component, error code: "
        + std::to_string(ping_res)).c_str());
      return 1;
    }

    timer_cnt = (timer_cnt + 1) % 5;
    if (timer_cnt != 0) { 
      continue; 
    }

    std::vector<arrival_info_t> arrivals;
  
    res = api_client::fetch_arrivals(apiKey, stationIdList, arrivals);
  
  
    res = bkk_screen_client_set_table_data(key, arrivals);
    if (res != BKK_SCREEN_ERROR_NONE) {
      log_error("Main", (
        "Failed to set table data, error code: "
        + std::to_string(res)).c_str());
      return 1;
    }


  }


  return 0;
}