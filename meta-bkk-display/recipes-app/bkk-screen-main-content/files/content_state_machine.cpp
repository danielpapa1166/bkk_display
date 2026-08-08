#include "content_state_machine.hpp"
#include "api_context.hpp"
#include "screen_context.hpp"

#include <cstddef>
#include <unistd.h>
#include <network_manager_pub.h>
#include <bkk_utils/bkk_dbus_broadcast_client.h>
#include <bkk_utils/bkk_dbus.h>
#include <rbuflogd/logger.h>


namespace content_sm {

// ----------------------------------------------------------------------------
// local function headers
// ----------------------------------------------------------------------------

static content_state_t get_current_state();
static int broadcast_signal_handler(
    const char *sigvalue, size_t sigvalue_len, void *user_data);
static int exec_normal_display_mode();

// ----------------------------------------------------------------------------
// local variables
// ----------------------------------------------------------------------------

static bc_client_t client;

static content_state_t current_state = content_state::INIT;
static network_manager_mode_t online_status = NETWORK_MANAGER_MODE_UNKNOWN;
static bool api_ready = false;
static const int max_retries = 3;

int init() {

  log_debug("Init", "Initializing content state machine, wait for DBUS ...");
  // check connection status: 
  (void) wait_for_dbus_connection(-1);
  log_debug("Init", "D-Bus connection established, proceeding with initialization.");

  int res; 
  int retry_counter = 0;
  static bkk_dbus_listener_t clt;
  const char * bus_name = NETWORK_MANAGER_DBUS_NAME;
  retry_counter = 0;
  do {
    res = init_broadcast_client(
      bus_name,
      DBUS_PEER_NAME,
      &clt,
      broadcast_signal_handler,
      &client,
      &client
    );
    if(res != 0) {
      log_warning("Init", "Failed to initialize broadcast client");
    }
    retry_counter++;
    if(retry_counter > max_retries) {
      log_error("Init", 
        "Exceeded max retries for initializing broadcast client");
      return -1;
    }
    sleep(2);
  } while(res != 0);


  // init screen context: 
  retry_counter = 0; 
  do {
    res = screen_ctx::init_screen_context();
    if(res != 0) {
      log_warning("Init", "Failed to initialize screen context");
    }
    retry_counter++;
    if(retry_counter > max_retries) {
      log_error("Init", 
        "Exceeded max retries for initializing screen context");
      return -1;
    }
  } while(res != 0);


  screen_ctx::put_screen_text("Initializing ...");
  sleep(1);

  switch_state(content_state::ACCESS_POINT_MODE);

  sleep(5);



  const content_state_t st = get_current_state();

  if(st == content_state::ACCESS_POINT_MODE) {
    log_info("Init", "Initial state: Access Point Mode");
    switch_state(content_state::ACCESS_POINT_MODE);
  }
  else if(st == content_state::CONFIG_API_MODE) {
    log_info("Init", "Initial state: Config API Mode");
    switch_state(content_state::CONFIG_API_MODE);
  }
  else if(st == content_state::NORMAL_DISPLAY_MODE) {
    log_info("Init", "Initial state: Normal Display Mode");
    switch_state(content_state::NORMAL_DISPLAY_MODE);
  }
  else {
    log_error("Init", "Failed to determine initial state, defaulting to Error Mode");
    switch_state(content_state::ERROR_MODE);
  }

  return 0;
}

int ping_timer_callback(void * arg) {

  if(current_state != content_state::ACCESS_POINT_MODE
      && current_state != content_state::CONFIG_API_MODE
      && current_state != content_state::NORMAL_DISPLAY_MODE
      && current_state != content_state::ERROR_MODE
    ) {
    log_debug("Ping", "Ping timer callback called, "
      "but not in a state that requires pinging the screen");
    return 0;
  }

  (void)arg;
  const int ping_res = screen_ctx::send_screen_ping();
  if(ping_res != 0) {
    log_error("Ping", "Failed to send ping to screen");
    return -1;
  }
  return 0;
}


int reinit() {
  const content_state_t st = get_current_state();

  if(st == content_state::ACCESS_POINT_MODE) {
    log_info("Reinit", "Reinitializing state: Access Point Mode");
    switch_state(content_state::ACCESS_POINT_MODE);
  }
  else if(st == content_state::CONFIG_API_MODE) {
    log_info("Reinit", "Reinitializing state: Config API Mode");
    switch_state(content_state::CONFIG_API_MODE);
  }
  else if(st == content_state::NORMAL_DISPLAY_MODE) {
    log_info("Reinit", "Reinitializing state: Normal Display Mode");
    switch_state(content_state::NORMAL_DISPLAY_MODE);
  }
  else {
    log_error("Reinit", "Failed to determine reinitialization state, defaulting to Error Mode");
    switch_state(content_state::ERROR_MODE);
  }

  return 0;
}


int exec_fun() {
  if(current_state == content_state::NORMAL_DISPLAY_MODE) {
    return exec_normal_display_mode();
  }
  return 0;
}

int switch_state(content_state_t new_state) {

  if(current_state == new_state) {
    return 0;
  }

  if(new_state == content_state::ACCESS_POINT_MODE) {
    screen_ctx::switch_context(screen_ctx::context_state::DISPLAY_HELPER);
    
    screen_ctx::put_helper_info(
      "Access Point Mode",
      {"Bla bla",
       "Blablabla"}
    );
  }
  else if(new_state == content_state::CONFIG_API_MODE) {
    screen_ctx::switch_context(screen_ctx::context_state::DISPLAY_HELPER);
    screen_ctx::put_helper_info(
      "Config API Mode",
      {"Bla bla",
       "Blablabla"}
    );
  }
  else if(new_state == content_state::NORMAL_DISPLAY_MODE) {
    screen_ctx::switch_context(screen_ctx::context_state::DISPLAY_ARRIVAL);
  }
  else {
    log_error("StateMachine", "Invalid state transition requested");
    return -1;
  }

  current_state = new_state;
  return 0;
}


int exit() {
  screen_ctx::switch_context(screen_ctx::context_state::REPORT_STATUS);
  screen_ctx::put_screen_text("Terminating ...");
  sleep(1);
  screen_ctx::switch_context(screen_ctx::context_state::RELEASE_COMPONENT);
  return 0;
}

// ----------------------------------------------------------------------------
// local function definitions
// ----------------------------------------------------------------------------

static content_state_t get_current_state() {

  // fetch online status from network manager: 
  bc_client_request_t request;
  request.request[0] = 'a';
  request.request[1] = '\0'; 
  const char * bus_name = NETWORK_MANAGER_DBUS_NAME;
  bc_data_un response;
  int res; 
  int retry_counter = 0; 
  do {
    retry_counter++; 
    res = send_client_request(
      &client, &request, &(response.bc_server_data));
    if (res != 0 && retry_counter > max_retries) {
      log_error("Main", "Failed to request the initial network mode");
      return content_state::ERROR_MODE;
    }
    sleep(2);
  } while(res != 0); 

  const network_manager_mode_t online_st = response.network_manager_data.mode;
  
  // try to load API context:
  res = load_api_context(); 
  const bool api_ready = (bool)(res == 0);
    
  log_debug("GetState", ("Online status: " + std::to_string(online_st) +
    ", API context load result: " + std::to_string(res) +
    ", API ready: " + std::to_string(api_ready)).c_str());

  if(online_st == NETWORK_MANAGER_MODE_ACCESS_POINT) {
    return content_state::ACCESS_POINT_MODE;
  }
  else if(online_st == NETWORK_MANAGER_MODE_WIFI_CLIENT) {
    if(api_ready) {
      return content_state::NORMAL_DISPLAY_MODE;
    }
    else {
      return content_state::CONFIG_API_MODE;
    }
  }
  else {
    log_warning("Init", "Unknown online status, defaulting to CONFIG_API_MODE");
    return content_state::ERROR_MODE;
  }

  return content_state::ERROR_MODE;
}


static int broadcast_signal_handler(
    const char *sigvalue, size_t sigvalue_len, void *user_data) {

  (void)user_data;

  network_manager_data_t *received_data = (network_manager_data_t*)sigvalue;

  if(received_data == nullptr || sigvalue_len == 0) {
    log_warning("Broadcast", "Received null broadcast signal");
    return -1;
  }

  printf("Received broadcast signal: Network mode changed to: %s\n", 
    (received_data->mode == NETWORK_MANAGER_MODE_ACCESS_POINT) ? "AP" : 
    (received_data->mode == NETWORK_MANAGER_MODE_WIFI_CLIENT) ? "WiFi Client" : 
    "Unknown");

  if (received_data->mode == NETWORK_MANAGER_MODE_ACCESS_POINT) {
    online_status = NETWORK_MANAGER_MODE_ACCESS_POINT;
    log_info("Broadcast", "Received broadcast signal: Network mode changed to Access Point");

    switch_state(content_state::ACCESS_POINT_MODE);
  }
  else if (received_data->mode == NETWORK_MANAGER_MODE_WIFI_CLIENT) {
    online_status = NETWORK_MANAGER_MODE_WIFI_CLIENT;
    log_info("Broadcast", "Received broadcast signal: Network mode changed to WiFi Client");
    if(api_ready) {
      switch_state(content_state::NORMAL_DISPLAY_MODE);
    }
    else {
      switch_state(content_state::CONFIG_API_MODE);
    }
  }
  else {
    log_info("Broadcast", "Received broadcast signal: Network mode changed to Unknown");
  }

  return 0;
}



static int exec_normal_display_mode() {
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

    screen_ctx::switch_context(screen_ctx::context_state::REPORT_STATUS);
    screen_ctx::put_screen_text("Error: Fetch failed. \nLocal: " 
      + api_response.local_server_status 
      + "\nRemote: " 
      + api_response.remote_server_status);
    return 0;
  }

  if(arrivals.empty()) {
    log_info("Update", "No arrival data available");
    screen_ctx::switch_context(screen_ctx::context_state::REPORT_STATUS);
    screen_ctx::put_screen_text("No arrival data\nfor the selected stations ");
    return 0;
  }
  else {
    screen_ctx::switch_context(screen_ctx::context_state::DISPLAY_ARRIVAL);
    const int send_res = screen_ctx::send_arrival_info(arrivals);
    if(send_res != 0) {
      log_error("Update", "Failed to send arrival info to the screen");
      return 0;
    }
  }
  return 0; 
}


} // namespace content_sm

