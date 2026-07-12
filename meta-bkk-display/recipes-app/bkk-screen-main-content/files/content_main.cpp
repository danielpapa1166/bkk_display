#include "bkk_screen_client/client.hpp"
#include "bkk_api_client.hpp"
#include <bkk_screen_client/common_defs.hpp>
#include <ctime>
#include <rbuflogd/logger.h>
#include <bkk_utils/bkk_utils_timing.h>
#include <vector>
#include <string>
#include <cstring>
#include <pthread.h>


// ----------------------------------------------------------------------------
// Local data structures and functions for managing main content context
// ----------------------------------------------------------------------------

typedef struct {
  pthread_mutex_t mutex;
  bkk_screen_component_id_t component_id;
  int component_key;
} main_content_ctx_t;

typedef struct {
  pthread_mutex_t mutex;
  std::string api_key;
  std::vector<std::string> station_id_list;
  std::vector<std::string> station_name_list;
} api_fetch_context_t;


static main_content_ctx_t main_content_ctx = {
  .mutex = PTHREAD_MUTEX_INITIALIZER,
  .component_id = BKK_SCREEN_COMPONENT_MAX, // invalid
  .component_key = -1
};

static api_fetch_context_t api_fetch_ctx = {
  .mutex = PTHREAD_MUTEX_INITIALIZER,
  .api_key = "",
  .station_id_list = {},
  .station_name_list = {}
};

// ----------------------------------------------------------------------------
// Local helper function headers: 
// ----------------------------------------------------------------------------
static int set_main_content_ctx(
    const bkk_screen_component_id_t * const component_id, 
    const int * const component_key,
    main_content_ctx_t * const ctx);
static int get_main_content_ctx(
    main_content_ctx_t * const ctx,
    bkk_screen_component_id_t * const component_id, 
    int * const component_key);
static int set_api_fetch_ctx(
    const std::string * const api_key,
    const std::vector<std::string> * const station_id_list,
    const std::vector<std::string> * const station_name_list,
    api_fetch_context_t * const ctx);
static std::string get_api_key(api_fetch_context_t * const ctx);
static std::vector<std::string> get_station_id_list(
    api_fetch_context_t * const ctx);
static int init_api_context();

// ----------------------------------------------------------------------------
// local timer callback function headers for main content updates
// ----------------------------------------------------------------------------
static int send_ping(void * arg);
static int send_arrival_data(void * arg);

// ----------------------------------------------------------------------------
// main: 
// ----------------------------------------------------------------------------

int main() {
  rbuflogd_logger_init("ScrCltMc");
  log_info("Main", "Starting BKK Screen Client");
  
  // --------------------------------------------------------------------------
  // Acquire the main content component as a status screen: 
  // --------------------------------------------------------------------------

  int res, key; 
  bkk_screen_component_id_t component_id = BKK_SCREEN_COMPONENT_STATUS_SCREEN;
  res = bkk_screen_client_acquire_component(
    component_id, 
    &key
  );

  if (res != BKK_SCREEN_ERROR_NONE) {
    log_error("Main", (
      "Failed to acquire screen component, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }

  set_main_content_ctx(
    &component_id, &key, &main_content_ctx);

  // put out something: 
  const char * status_text = "Backend init ... ";
  res = bkk_screen_client_set_status_screen_data(
    key, status_text, strlen(status_text)
  );
  // --------------------------------------------------------------------------
  // init backend: 
  // --------------------------------------------------------------------------

  pthread_mutex_init(&main_content_ctx.mutex, NULL);
  pthread_mutex_init(&api_fetch_ctx.mutex, NULL);

  const int api_init_res = init_api_context(); 
  if(api_init_res < 0) {
    log_error("Main", "Failed to initialize API context");
    return 1;
  }

  // --------------------------------------------------------------------------
  // Setup timer for pinging the main content component
  // --------------------------------------------------------------------------

  timer_thread_ctx_t ping_timer_ctx = {
    .config = {
      .timer_fd = -1,
      .cyclic_expiration_sec = 1,
      .cyclic_expiration_nsec = 0,
      .initial_expiration_sec = 1,
      .initial_expiration_nsec = 0,
    },
    .callback = send_ping,
    .arg = &main_content_ctx,
  };

  res = bkk_setup_timer_with_callback(&ping_timer_ctx);
  if (res != TIMER_ERROR_NONE) {
    log_error("Init", (
      "Failed to setup ping timer, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }
  
  sleep(1);

  // put out something else: 
  status_text = "Init done, running ... ";
  res = bkk_screen_client_set_status_screen_data(
    key, status_text, strlen(status_text)
  );

  sleep(1);

  // --------------------------------------------------------------------------
  // Release the main content component and acquire the table component
  // --------------------------------------------------------------------------

  bkk_screen_client_release_screen_component(
    key, component_id); 
    
  component_id = BKK_SCREEN_COMPONENT_TABLE;
  res = bkk_screen_client_acquire_component(
    component_id,
    &key);

  set_main_content_ctx(
    &component_id, &key, &main_content_ctx);

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

  // --------------------------------------------------------------------------
  // Setup timer for updating the table component with arrival data
  // --------------------------------------------------------------------------

  timer_thread_ctx_t arrival_data_timer_ctx = {
    .config = {
      .timer_fd = -1,
      .cyclic_expiration_sec = 5,
      .cyclic_expiration_nsec = 0,
      .initial_expiration_sec = 1,
      .initial_expiration_nsec = 0,
    },
    .callback = send_arrival_data,
    .arg = &main_content_ctx,
  };

  res = bkk_setup_timer_with_callback(&arrival_data_timer_ctx);
  if (res != TIMER_ERROR_NONE) {
    log_error("Init", (
      "Failed to setup arrival data timer, error code: "
      + std::to_string(res)).c_str());
    return 1;
  }
  bkk_join_timer_with_callback(&arrival_data_timer_ctx);

  log_info("Main", "Arrival data timer thread has finished, exiting main loop");


  // --------------------------------------------------------------------------
  // wait for threads to finish
  // --------------------------------------------------------------------------

  bkk_join_timer_with_callback(&ping_timer_ctx);

  bkk_screen_client_release_screen_component(
    key, component_id);

  log_info("Main", "Exiting BKK Screen Client");

  return 0;
}


// ----------------------------------------------------------------------------
// Local helper function implementations: 
// ----------------------------------------------------------------------------

static int set_main_content_ctx(
    const bkk_screen_component_id_t * const component_id, 
    const int * const component_key,
    main_content_ctx_t * const ctx) {

  if(ctx == NULL) {
    return -1;
  }

  pthread_mutex_lock(&ctx->mutex);
  if(component_id != NULL) {
    ctx->component_id = *component_id;
  }
  if(component_key != NULL) {
    ctx->component_key = *component_key;
  }
  pthread_mutex_unlock(&ctx->mutex);
  return 0;
}

static int get_main_content_ctx(
    main_content_ctx_t * const ctx,
    bkk_screen_component_id_t * const component_id, 
    int * const component_key) {
  
  if(ctx == NULL) {
    return -1;
  }
  
  if(component_id == NULL || component_key == NULL) {
    return -1;
  }
  pthread_mutex_lock(&ctx->mutex);
  *component_id = ctx->component_id;
  *component_key = ctx->component_key;
  pthread_mutex_unlock(&ctx->mutex);
  return 0;
}

static int set_api_fetch_ctx(
    const std::string * const api_key,
    const std::vector<std::string> * const station_id_list,
    const std::vector<std::string> * const station_name_list,
    api_fetch_context_t * const ctx) {

  if(ctx == NULL) {
    return -1;
  }

  pthread_mutex_lock(&ctx->mutex);
  if(api_key != NULL) {
    ctx->api_key = *api_key;
  }
  if(station_id_list != NULL) {
    ctx->station_id_list = *station_id_list;
  }
  if(station_name_list != NULL) {
    ctx->station_name_list = *station_name_list;
  }
  pthread_mutex_unlock(&ctx->mutex);
  return 0;
}

static std::string get_api_key(api_fetch_context_t * const ctx) {
  if(ctx == NULL) {
    return "";
  }
  pthread_mutex_lock(&ctx->mutex);
  std::string api_key = ctx->api_key;
  pthread_mutex_unlock(&ctx->mutex);
  return api_key;
}

static std::vector<std::string> get_station_id_list(
    api_fetch_context_t * const ctx) {
  if(ctx == NULL) {
    return {};
  }
  pthread_mutex_lock(&ctx->mutex);
  std::vector<std::string> station_id_list = ctx->station_id_list;
  pthread_mutex_unlock(&ctx->mutex);
  return station_id_list;
}

static std::vector<std::string> get_station_name_list(
    api_fetch_context_t * const ctx) {
  if(ctx == NULL) {
    return {};
  }
  pthread_mutex_lock(&ctx->mutex);
  std::vector<std::string> station_name_list = ctx->station_name_list;
  pthread_mutex_unlock(&ctx->mutex);
  return station_name_list;
}

static int init_api_context() {

  std::string apiKey;
  int res = api_client::load_api_key(apiKey);

  if (res != 0) {
    log_error("Update", (
      "Failed to load API key, error code: "
      + std::to_string(res)).c_str());
    return -1;
  }

  std::vector<std::string> stationIdList;
  std::vector<std::string> stationNameList;

  res = api_client::load_station_ids(
    stationIdList, 
    stationNameList
  );

  if (res != 0) {
    log_error("Update", (
      "Failed to load station IDs, error code: "
      + std::to_string(res)).c_str());
    return -1;
  }

  set_api_fetch_ctx(
    &apiKey, 
    &stationIdList, 
    &stationNameList, 
    &api_fetch_ctx);
  return 0;
}

// ----------------------------------------------------------------------------
// local timer callback function implementations for main content updates
// ----------------------------------------------------------------------------


static int send_ping(void * arg) {
  main_content_ctx_t * ctx = static_cast<main_content_ctx_t *>(arg);
  int key;
  bkk_screen_component_id_t component_id; 

  const int res = get_main_content_ctx(ctx, 
    &component_id, &key);

  if(res < 0) {
    log_error("Ping", "Failed to get main content context");
    return -1;
  }

  const bkk_screen_error_code_t ping_res 
    = bkk_screen_client_ping(key, component_id);

  if (ping_res != BKK_SCREEN_ERROR_NONE) {
    log_error("Ping", (
      "Failed to send ping for component, error code: " 
      + std::to_string(ping_res)).c_str());
    return -1;
  }
  return 0;
}


static int send_arrival_data(void * arg) {
  main_content_ctx_t * ctx = static_cast<main_content_ctx_t *>(arg);

  log_info("Update", "Fetching arrival data from API and sending it to the screen");

  int component_key;
  bkk_screen_component_id_t component_id; 

  const int ctx_res = get_main_content_ctx(ctx, 
    &component_id, &component_key);

  if(ctx_res < 0) {
    log_error("Update", "Failed to get main content context");
    return -1;
  }

  std::vector<arrival_info_t> arrivals;
  std::string apiKey = get_api_key(&api_fetch_ctx);
  std::vector<std::string> stationIdList 
    = get_station_id_list(&api_fetch_ctx);
  std::vector<std::string> stationNameList 
    = get_station_name_list(&api_fetch_ctx);

  const int fetch_res = api_client::fetch_arrivals(
    apiKey, 
    stationIdList, 
    stationNameList, 
    arrivals);
    
  if (fetch_res != 0) {
    log_error("Update", (
      "Failed to fetch arrival data, error code: "
      + std::to_string(fetch_res)).c_str());
    return -1;
  }

  log_info("Update", (
    "Fetched arrival data for " 
    + std::to_string(arrivals.size()) 
    + " stations, sending it to the screen: ").c_str()
  );
  const int set_res = bkk_screen_client_set_table_data(component_key, arrivals);
  if (set_res != BKK_SCREEN_ERROR_NONE) {
    log_error("Update", (
      "Failed to set table data, error code: "
      + std::to_string(set_res)).c_str());
    return -1;
  }

  log_info("Update", "Successfully sent arrival data to the screen");
  return 0;

}


