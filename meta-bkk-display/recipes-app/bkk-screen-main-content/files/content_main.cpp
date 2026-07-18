#include <bkk_screen_client/common_defs.hpp>
#include <bkk_screen_client/client.hpp>
#include <bkk_utils/bkk_utils_timing.h>
#include <bkk_utils/bkk_utils_online_status.h>
#include <rbuflogd/logger.h>

#include "bkk_api_client.hpp"
#include "api_context.hpp"
#include "screen_context.hpp"

#include <ctime>
#include <vector>
#include <string>
#include <cstring>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/eventfd.h>


// ----------------------------------------------------------------------------
// local function headers
// ----------------------------------------------------------------------------

static void connect_signal_handlers();
static int ping_timer_callback(void * arg);
static void terminate_signal_handler(int signum);
static void udpate_signal_handler(int signum);

// ----------------------------------------------------------------------------
// local variables 
// ----------------------------------------------------------------------------

timer_thread_ctx_t ping_timer = {
  .config = {
    .timer_fd = -1,
    .cyclic_expiration_sec = 1,
    .cyclic_expiration_nsec = 0,
    .initial_expiration_sec = 1,
    .initial_expiration_nsec = 0,
  },
  .callback = ping_timer_callback,
  .arg = nullptr,
};

static volatile sig_atomic_t g_update_requested = 0;
static int g_update_event_fd = -1;



// ----------------------------------------------------------------------------
// main: 
// ----------------------------------------------------------------------------

int main() {
  rbuflogd_logger_init("ScrCltMc");
  log_info("Main", "Starting BKK Screen Client");

  connect_signal_handlers();

  const int context_init_res = init_screen_context();
  if(context_init_res != 0) {
    return 1;
  }

  // start ping timer thread
  int res = bkk_setup_timer_with_callback(&ping_timer);
  if (res != TIMER_ERROR_NONE) {
    log_error("Init", (
      "Failed to setup ping timer, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }


  // check online status here (AP or wifi connection mode) 
  put_screen_text("Checking online status ...");
  // const online_status_t online_status = true; // is_online(); 
  sleep(1); 
  put_screen_text("Done");
  // ...

  const int load_res = init_api_context();
  if(load_res != 0) {
    log_error("Main", "Failed to load API context");
    return 1;
  }


  timer_config_t main_content_timer_config = {
    .timer_fd = -1,
    .cyclic_expiration_sec = 5,
    .cyclic_expiration_nsec = 0,
    .initial_expiration_sec = 1,
    .initial_expiration_nsec = 0,
  };

  timer_error_t timer_setup_res = bkk_setup_timer(&main_content_timer_config);
  if (timer_setup_res != TIMER_ERROR_NONE) {
    log_error("Init", (
      "Failed to setup main content timer, error code: "
      + std::to_string(timer_setup_res)).c_str());  
    return 1;
  }

  g_update_event_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (g_update_event_fd < 0) {
    log_error("Init", "Failed to create update event fd");
    return 1;
  }

  switch_context(context_state::DISPLAY_ARRIVAL);


  while(1) {
    // ------------------------------------------------------------------------
    // wait for either periodic timer or update signal
    // ------------------------------------------------------------------------
    fd_set readfds;

    const int max_fd = main_content_timer_config.timer_fd > g_update_event_fd
      ? main_content_timer_config.timer_fd
      : g_update_event_fd;

    int wait_res = -1;
    while (1) {
      FD_ZERO(&readfds);
      FD_SET(main_content_timer_config.timer_fd, &readfds);
      FD_SET(g_update_event_fd, &readfds);

      wait_res = select(
        max_fd + 1, 
        &readfds, 
        NULL, 
        NULL, 
        NULL);

      if (wait_res < 0 && errno == EINTR) {
        continue;
      }
      break;
    }

    if (wait_res < 0) {
      log_error("Main", (
        "Failed to wait on main content timer, errno: "
        + std::to_string(errno)).c_str());

      switch_context(context_state::REPORT_STATUS);
      put_screen_text("Error: Timer wait failed");
      continue;
    }

    // ------------------------------------------------------------------------
    // timer interrupt occurred, read the timer fd to clear the interrupt
    // ------------------------------------------------------------------------
    if (FD_ISSET(main_content_timer_config.timer_fd, &readfds)) {
      uint64_t expirations = 0;
      ssize_t timer_read_res = -1;
      while (1) {
        timer_read_res = read(
          main_content_timer_config.timer_fd,
          &expirations,
          sizeof(expirations));
        if (timer_read_res < 0 && errno == EINTR) {
          continue;
        }
        break;
      }

      if (timer_read_res != (ssize_t)sizeof(expirations)) {
        log_error("Main", "Failed to read main content timer fd");
        continue;
      }
    }

    // ------------------------------------------------------------------------
    // user signal received 
    // ------------------------------------------------------------------------
    if (FD_ISSET(g_update_event_fd, &readfds)) {
      uint64_t wake_counter = 0;
      ssize_t wake_read_res = -1;
      while (1) {
        wake_read_res = read(
          g_update_event_fd, 
          &wake_counter, 
          sizeof(wake_counter));
        if (wake_read_res < 0 && errno == EINTR) {
          continue;
        }
        break;
      }

      if (wake_read_res < 0 && errno != EAGAIN) {
        log_error("Main", "Failed to read update event fd");
      }
      g_update_requested = 1;
    }

    if (g_update_requested != 0) {
      g_update_requested = 0;
      log_info("Main", "Received update signal (SIGUSR1), reloading API context");
      const int reload_res = load_api_context();
      if (reload_res != 0) {
        log_error("Update", "Failed to reload API context");
      }
    }

    // ------------------------------------------------------------------------
    // in either case, fetch and send arrival data to the screen
    // ------------------------------------------------------------------------
    std::vector<arrival_info_t> arrivals;
    std::string apiKey = get_api_key();
    std::vector<std::string> stationIdList 
      = get_station_id_list();
    std::vector<std::string> stationNameList 
      = get_station_name_list();

    api_client::api_fetch_status_t api_response = api_client::fetch_arrivals(
      apiKey, 
      stationIdList, 
      stationNameList, 
      arrivals);
      
    if (api_response.any_error != 0) {
      log_error("Update", (
        "Failed to fetch arrival data, error code: "
        + api_response.local_server_status
        + ", remote server status: "
        + api_response.remote_server_status).c_str());

      switch_context(context_state::REPORT_STATUS);
      put_screen_text("Error: Fetch failed. \nLocal: " 
        + api_response.local_server_status 
        + "\nRemote: " 
        + api_response.remote_server_status);
      continue;
    }

    if(arrivals.empty()) {
      log_info("Update", "No arrival data available");
      switch_context(context_state::REPORT_STATUS);
      put_screen_text("No arrival data\nfor the selected stations ");
      continue;
    }
    else {
      switch_context(context_state::DISPLAY_ARRIVAL);
      const int send_res = send_arrival_info(arrivals);
      if(send_res != 0) {
        log_error("Update", "Failed to send arrival info to the screen");
        continue;
      }
    }
  } // end of main loop

  log_error("Main", "Exiting main loop due to error, cleaning up ...");
  bkk_cleanup_timer_with_callback(&ping_timer);
  sleep(1);

  // wait for threads to finish
  bkk_join_timer_with_callback(&ping_timer);
  if (g_update_event_fd >= 0) {
    (void)close(g_update_event_fd);
    g_update_event_fd = -1;
  }
  switch_context(context_state::RELEASE_COMPONENT);

  log_info("Main", "Exiting BKK Screen Client");

  return 0;
} // end of main


// ----------------------------------------------------------------------------
// local function definitions
// ----------------------------------------------------------------------------

static void connect_signal_handlers() {
  struct sigaction sa_terminate;
  memset(&sa_terminate, 0, sizeof(sa_terminate));

  sa_terminate.sa_handler = terminate_signal_handler;
  sigaction(SIGINT, &sa_terminate, NULL);
  sigaction(SIGTERM, &sa_terminate, NULL);

  struct sigaction sa_update;
  memset(&sa_update, 0, sizeof(sa_update));
  sa_update.sa_handler = udpate_signal_handler;
  sa_update.sa_flags = SA_RESTART;
  sigaction(SIGUSR1, &sa_update, NULL);
}


static int ping_timer_callback(void * arg) {
  send_screen_ping();
  return 0;
}



static void terminate_signal_handler(int signum) {
  switch_context(context_state::REPORT_STATUS);
  put_screen_text("Terminating ...");
  log_info("Main", (
    "Received termination signal: "
    + std::to_string(signum)).c_str()
  ); 
  sleep(1);
  switch_context(context_state::RELEASE_COMPONENT);
  bkk_cleanup_timer_with_callback(&ping_timer);
  bkk_join_timer_with_callback(&ping_timer);
  exit(0);
}

static void udpate_signal_handler(int signum) {
  (void)signum;
  g_update_requested = 1;
  if (g_update_event_fd >= 0) {
    const uint64_t wake_signal = 1;
    (void)write(g_update_event_fd, &wake_signal, sizeof(wake_signal));
  }
}


