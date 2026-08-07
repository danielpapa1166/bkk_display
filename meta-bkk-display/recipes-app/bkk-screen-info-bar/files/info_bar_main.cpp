#include "online_check.hpp"
#include "clock_update.hpp"

#include <rbuflogd/logger.h>
#include <bkk_screen_client/client.hpp>
#include <bkk_utils/bkk_utils_timing.h>

#include <curl/multi.h>
#include <cstddef>
#include <cstdio>
#include <string>
#include <signal.h>
#include <cstring>
#include <unistd.h>

// ----------------------------------------------------------------------------
// local timer callback functions for info bar updates
// ----------------------------------------------------------------------------

static void terminate_signal_handler(int signum);
static int send_ping(void * arg);
static int send_info_bar_data(void * arg);


// ----------------------------------------------------------------------------
// local variables
// ----------------------------------------------------------------------------

static int info_bar_key = 0;

static timer_thread_ctx_t info_bar_ping_timer_ctx = {
  .config = {
    .timer_fd = -1,
    .cyclic_expiration_sec = 1,
    .cyclic_expiration_nsec = 0,
    .initial_expiration_sec = 1,
    .initial_expiration_nsec = 0,
  },
  .callback = send_ping,
  .arg = &info_bar_key,
};

static timer_thread_ctx_t info_bar_update_data_timer_ctx = {
  .config = {
    .timer_fd = -1,
    .cyclic_expiration_sec = 1,
    .cyclic_expiration_nsec = 0,
    .initial_expiration_sec = 1,
    .initial_expiration_nsec = 0,
  },
  .callback = send_info_bar_data,
  .arg = &info_bar_key,
};


// ----------------------------------------------------------------------------
// Main function
// ----------------------------------------------------------------------------

int main() {
  // --------------------------------------------------------------------------
  // Initialize logger and online check
  // --------------------------------------------------------------------------
  rbuflogd_logger_init("ScrCltIb");
  online_check::online_check_init();

  (void) online_check::is_online(); 

  // --------------------------------------------------------------------------
  // Acquire the info bar component
  // --------------------------------------------------------------------------

  int res; 
  int attempts_cnt = 0;
  do {
    res = bkk_screen_client_acquire_component(
      BKK_SCREEN_COMPONENT_INFO_BAR, 
      &info_bar_key);

    sleep(1);
    attempts_cnt++;
  } while (res != BKK_SCREEN_ERROR_NONE && attempts_cnt < 5);

  if (res != BKK_SCREEN_ERROR_NONE) {
    log_error("Init", (
      "Failed to acquire screen component, error code: " 
      + std::to_string(res)).c_str());
    return 1;
  }
  else {
    log_info("Init", (
      "Successfully acquired screen component, key: " 
      + std::to_string(info_bar_key)).c_str());
  }


  // --------------------------------------------------------------------------
  // Setup timer for info bar updates
  // --------------------------------------------------------------------------

  res = bkk_setup_timer_with_callback(&info_bar_ping_timer_ctx);
  if (res != TIMER_ERROR_NONE) {
    log_error("Init", (
      "Failed to setup info bar ping timer, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }

  res = bkk_setup_timer_with_callback(&info_bar_update_data_timer_ctx);
  if (res != TIMER_ERROR_NONE) {
    log_error("Init", (
      "Failed to setup info bar update data timer, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }


  // --------------------------------------------------------------------------
  // join the timers and wait for them to finish
  // --------------------------------------------------------------------------

  res = bkk_join_timer_with_callback(&info_bar_ping_timer_ctx);
  if (res != TIMER_ERROR_NONE) {
    log_error("Join", (
      "Failed to join info bar ping timer, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }
  res = bkk_join_timer_with_callback(&info_bar_update_data_timer_ctx);
  if (res != TIMER_ERROR_NONE) {
    log_error("Join", (
      "Failed to join info bar update data timer, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }

  return 0;
}


static void connect_signal_handlers() {
  struct sigaction sa_terminate;
  memset(&sa_terminate, 0, sizeof(sa_terminate));

  sa_terminate.sa_handler = terminate_signal_handler;
  sigaction(SIGINT, &sa_terminate, NULL);
  sigaction(SIGTERM, &sa_terminate, NULL);

}



static void terminate_signal_handler(int signum) {

  log_info("Main", (
    "Received termination signal: "
    + std::to_string(signum)).c_str()
  ); 
  
  bkk_screen_error_code_t release_err = bkk_screen_client_release_screen_component(
    info_bar_key, BKK_SCREEN_COMPONENT_INFO_BAR);

  if(release_err != BKK_SCREEN_ERROR_NONE) {
    log_error("Terminate", (
      "Failed to release screen component, error code: "
      + std::to_string(release_err)).c_str());
  }
  else {
    log_info("Terminate", "Successfully released screen component");
  }

  bkk_cleanup_timer_with_callback(&info_bar_ping_timer_ctx);
  bkk_join_timer_with_callback(&info_bar_ping_timer_ctx);
  bkk_cleanup_timer_with_callback(&info_bar_update_data_timer_ctx);
  bkk_join_timer_with_callback(&info_bar_update_data_timer_ctx);
  _exit(0);
}


static int send_ping(void * arg) {
  int key = *static_cast<int *>(arg);
  const bkk_screen_error_code_t ping_res 
    = bkk_screen_client_ping(key, BKK_SCREEN_COMPONENT_INFO_BAR);

  if (ping_res != BKK_SCREEN_ERROR_NONE) {
    log_error("Ping", (
      "Failed to send ping for component, error code: " 
      + std::to_string(ping_res)).c_str());
    return -1;
  }
  return 0;
}

static int send_info_bar_data(void * arg) {
  int key = *static_cast<int *>(arg);
  static std::string last_time;
  static bkk_screen_online_status_t last_online_status 
    = BKK_SCREEN_ONLINE_STATUS_OFFLINE;
  static std::string last_ip_address;

  std::string current_time;
  const int time_res = screen_clock_update::get_current_time_CET(
    current_time);
  if (time_res != 0) {
    log_error("Update", "Failed to get current time");
    return -1;
  }

  bkk_screen_online_status_t online_status 
    = online_check::is_online() 
    ? BKK_SCREEN_ONLINE_STATUS_ONLINE 
    : BKK_SCREEN_ONLINE_STATUS_OFFLINE;

  std::string ip_address;
  const int ip_res = online_check::get_ip_address(ip_address);
  if (ip_res != 0) {
    log_warning("Update", "Failed to get IP address");
    ip_address = "Unavailable";
  }

  if(current_time != last_time 
    || online_status != last_online_status
    || ip_address != last_ip_address) {
    const bkk_screen_error_code_t set_res = bkk_screen_client_set_info_bar_data(
      key, online_status, current_time.c_str(), ip_address.c_str());
    if (set_res != BKK_SCREEN_ERROR_NONE) {
      log_error("Update", (
        "Failed to set info bar data, error code: " 
        + std::to_string(set_res)).c_str());
      return -1;
    }
    last_time = current_time;
    last_online_status = online_status;
    last_ip_address = ip_address;
  }

  return 0;
}