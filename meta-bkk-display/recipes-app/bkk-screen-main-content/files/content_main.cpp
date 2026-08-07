#include <bkk_screen_client/common_defs.hpp>
#include <bkk_screen_client/client.hpp>
#include <bkk_utils/bkk_dbus_broadcast_client.h>
#include <bkk_utils/bkk_utils_timing.h>
#include <bkk_utils/bkk_utils_online_status.h>
#include <config_server_pub.h>
#include <rbuflogd/logger.h>


#include "bkk_api_client.hpp"
#include "api_context.hpp"
#include "screen_context.hpp"
#include "content_state_machine.hpp"

#include <ctime>
#include <atomic>
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
static void terminate_signal_handler(int signum);
static int setup_config_server_client();
static int config_server_signal_handler(
  const char *sigvalue, size_t sigvalue_len, void *user_data);
static int test_config_server_connection();
static void request_content_update();


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
  .callback = content_sm::ping_timer_callback,
  .arg = nullptr,
};

static std::atomic_bool g_update_requested{false};
static int g_update_event_fd = -1;
static bc_client_t g_config_server_client;
static bkk_dbus_listener_t g_config_server_listener;


// ----------------------------------------------------------------------------
// main: 
// ----------------------------------------------------------------------------

int main() {

  connect_signal_handlers();

  rbuflogd_logger_init("ScrCltMc");
  log_info("Main", "Starting BKK Screen Client");

  char ipv4[INET_ADDRSTRLEN]; 
  const ip_add_status_t ip_status = fetch_ip_addr("wlan0", ipv4);
  if (ip_status == IP_ADD_STATUS_HAS_IP) {
    printf("wlan0 IPv4 address: %s\n", ipv4);
  } 
  else {
    printf("wlan0 does not currently have an IPv4 address\n");
  }

  g_update_event_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (g_update_event_fd < 0) {
    log_error("Init", "Failed to create update event fd");
    return 1;
  }

  if (setup_config_server_client() != 0) {
    log_error("Init", "Failed to initialize config-server broadcast client");
    return 1;
  }

  if (test_config_server_connection() != 0) {
    log_error("Init", "Config-server handshake failed");
    return 1;
  }

  const int sm_init_res = content_sm::init();

  if(sm_init_res != 0) {
    printf("Failed to initialize content state machine, "
      "error code: %d\n", sm_init_res);
    log_error("Main", "Failed to initialize content state machine");
    return 1;
  }
  log_info("Main", "Content state machine initialized, "
    "listening for network mode changes");


  // start ping timer thread
  int res = bkk_setup_timer_with_callback(&ping_timer);
  if (res != TIMER_ERROR_NONE) {
    log_error("Init", (
      "Failed to setup ping timer, error code: "
      + std::to_string(res)).c_str());
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

  while(1) {
    // ------------------------------------------------------------------------
    // wait for either periodic timer or config-server update
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
      log_warning("Main", (
        "Failed to wait on main content timer, errno: "
        + std::to_string(errno)).c_str());
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
    // config-server update received
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
    }

    if (g_update_requested.exchange(false)) {
      content_sm::reinit();
    }

    content_sm::exec_fun();

  } // end of main loop

  log_error("Main", "Exiting main loop due to error, cleaning up ...");
  bkk_cleanup_timer_with_callback(&ping_timer);

  // wait for threads to finish
  bkk_join_timer_with_callback(&ping_timer);
  (void)bkk_dbus_deinit_listener(&g_config_server_listener);
  if (g_update_event_fd >= 0) {
    (void)close(g_update_event_fd);
    g_update_event_fd = -1;
  }
  
  content_sm::exit(); 

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

}



static void terminate_signal_handler(int signum) {

  log_info("Main", (
    "Received termination signal: "
    + std::to_string(signum)).c_str()
  ); 
  content_sm::exit();
  bkk_cleanup_timer_with_callback(&ping_timer);
  bkk_join_timer_with_callback(&ping_timer);
  exit(0);
}

static int setup_config_server_client() {
  const int init_res = init_broadcast_client(
    CONFIG_SERVER_DBUS_NAME,
    DBUS_PEER_NAME,
    &g_config_server_listener,
    config_server_signal_handler,
    nullptr,
    &g_config_server_client);
  return init_res;
}

static int config_server_signal_handler(
    const char *sigvalue, size_t sigvalue_len, void *user_data) {
  (void)user_data;

  const config_server_data_t *data =
    reinterpret_cast<const config_server_data_t *>(sigvalue);
  
  if(data == nullptr || sigvalue_len == 0) {
    log_warning("ConfigServer", "Received null config-server signal");
    return -1;
  }
  
  log_info("ConfigServer", (
    "Received config-server update, signal: "
    + std::to_string(static_cast<int>(data->signal))
    + " with length: "
    + std::to_string(sigvalue_len)).c_str());
  request_content_update();
  return 0;
}

static int test_config_server_connection() {
  bc_client_request_t request = {};
  request.request[0] = 'a';

  bc_config_server_un response = {};
  const int request_res = send_client_request(
    &g_config_server_client, &request, &response.bc_server_data);
  if (request_res != 0) {
    log_error("ConfigServer", "Failed to request config-server test data");
    return -1;
  }

  if (response.config_server_data.signal != CONFIG_SERVER_DONT_CARE) {
    log_error("ConfigServer", "Config-server returned an unexpected test signal");
    return -1;
  }

  log_info("ConfigServer", "Config-server connection established");
  return 0;
}

static void request_content_update() {
  g_update_requested.store(true);
  if (g_update_event_fd >= 0) {
    const uint64_t wake_signal = 1;
    (void)write(g_update_event_fd, &wake_signal, sizeof(wake_signal));
  }
}
