#include "screen_context.hpp"
#include <bkk_screen_client/common_defs.hpp>
#include <bkk_screen_client/client.hpp>
#include <bkk_utils/bkk_utils_timing.h>
#include <rbuflogd/logger.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <cstring>

namespace screen_ctx {
// ----------------------------------------------------------------------------
// Local data structures and functions for managing main content context
// ----------------------------------------------------------------------------

typedef struct {
  pthread_mutex_t mutex;
  bkk_screen_component_id_t component_id;
  int component_key;
} screen_content_ctx_t;



static screen_content_ctx_t screen_content_ctx = {
  .mutex = PTHREAD_MUTEX_INITIALIZER,
  .component_id = BKK_SCREEN_COMPONENT_MAX, // invalid
  .component_key = -1
};


main_context_state_t main_context_state = context_state::UNINITIALIZED;


// ----------------------------------------------------------------------------
// Local helper function headers: 
// ----------------------------------------------------------------------------
static int set_screen_content_ctx(
  const bkk_screen_component_id_t * const component_id, 
  const int * const component_key,
  screen_content_ctx_t * const ctx);
static int get_screen_content_ctx(
  screen_content_ctx_t * const ctx,
  bkk_screen_component_id_t * const component_id, 
  int * const component_key);

// ----------------------------------------------------------------------------
// interface functions for managing main content context
// ----------------------------------------------------------------------------

int init_screen_context() {

  pthread_mutex_init(&screen_content_ctx.mutex, NULL);
  // --------------------------------------------------------------------------
  // Acquire the main content component as a status screen: 
  // --------------------------------------------------------------------------

  const int st_res = switch_context(
    context_state::REPORT_STATUS);
  if(st_res < 0) {
    log_error("Main", "Failed to switch main context state to REPORT_STATUS");
    return -1;
  }
  return 0;
}

int send_screen_ping() {
  int key;
  bkk_screen_component_id_t component_id; 

  const int res = get_screen_content_ctx(
    &screen_content_ctx, 
    &component_id, 
    &key);

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


int switch_context(main_context_state_t new_state) {

  if(main_context_state == new_state) {
    return 0;
  }

  bkk_screen_component_id_t component_id = BKK_SCREEN_COMPONENT_MAX; // invalid
  int key = -1;

  get_screen_content_ctx(
    &screen_content_ctx, 
    &component_id, 
    &key);

  if(component_id < BKK_SCREEN_COMPONENT_MAX) {
    // release the current component before switching to a new state
    bkk_screen_client_release_screen_component(
      key, component_id);
  }

  // handle the RELEASE_COMPONENT state separately
  if(new_state == context_state::RELEASE_COMPONENT) {
    // set key and component_id to invalid values
    int key = -1;
    bkk_screen_component_id_t component_id = BKK_SCREEN_COMPONENT_MAX;
    set_screen_content_ctx(
      &component_id, &key, &screen_content_ctx);
    main_context_state = new_state;

    log_debug("Main", (
      "Released screen component and switched main context state to "
      + std::to_string(new_state)).c_str()
    );
    return 0;
  }

  // switch to the new state and acquire the corresponding component

  if(new_state == context_state::REPORT_STATUS) {
    component_id = BKK_SCREEN_COMPONENT_STATUS_SCREEN;
  } 
  else if(new_state == context_state::DISPLAY_HELPER) {
    component_id = BKK_SCREEN_COMPONENT_HELPER_SCREEN;
  }
  else if(new_state == context_state::DISPLAY_ARRIVAL) {
    component_id = BKK_SCREEN_COMPONENT_TABLE;
  } 
  else {
    log_error("Main", "Invalid new state for main context");
    return -1;
  }
    
  const int res = bkk_screen_client_acquire_component(
    component_id,
    &key);

  if(res != BKK_SCREEN_ERROR_NONE) {
    log_error("Main", (
      "Failed to acquire screen component, error code: "
      + std::to_string(res)).c_str());
    return -1;
  }

  set_screen_content_ctx(
    &component_id, &key, &screen_content_ctx);

  main_context_state = new_state;


  log_debug("Main", (
    "Switched main context state to "
    + std::to_string(new_state)).c_str()
  );  

  return 0; 
}


// put text on the screen, only allowed in REPORT_STATUS state
int put_screen_text(const std::string & text) {

  if(main_context_state != context_state::REPORT_STATUS) {
    log_error("Update", "Cannot put screen text when not in REPORT_STATUS state");
    return -1;
  }

  int key;
  bkk_screen_component_id_t component_id; 

  const int res = get_screen_content_ctx(
    &screen_content_ctx, 
    &component_id, &key);

  if(res < 0) {
    log_error("Update", "Failed to get main content context");
    return -1;
  }

  const bkk_screen_error_code_t set_res 
    = bkk_screen_client_set_status_screen_data(
      key, text.c_str(), text.length());

  if (set_res != BKK_SCREEN_ERROR_NONE) {
    log_error("Update", (
      "Failed to set status screen data, error code: " 
      + std::to_string(set_res)).c_str());
    return -1;
  }
  return 0;
}


int put_helper_info(const std::string & title, 
  const std::vector<std::string> & qr_command, 
  const std::vector<std::string> & text_lines) {

  if(main_context_state != context_state::DISPLAY_HELPER) {
    log_error("Update", "Cannot put helper info when not in DISPLAY_HELPER state");
    return -1;
  }

  int key;
  bkk_screen_component_id_t component_id; 

  const int res = get_screen_content_ctx(
    &screen_content_ctx, 
    &component_id, &key);

  if(res < 0) {
    log_error("Update", "Failed to get main content context");
    return -1;
  }

  helper_screen_data_t helper_data {};
  strncpy(
    helper_data.helper_title, 
    title.c_str(), 
    sizeof(helper_data.helper_title) - 1
  );
  helper_data.helper_title[sizeof(helper_data.helper_title) - 1] = '\0'; // Ensure null-termination

  size_t num_lines = std::min(
    text_lines.size(), 
    static_cast<size_t>(BKK_SCREEN_HELPER_MAX_NUM_OF_COLS)
  );
  helper_data.num_of_cols = static_cast<int>(num_lines);

  for (size_t i = 0; i < num_lines; ++i) {
    strncpy(
      helper_data.helper_text[i], 
      text_lines[i].c_str(), 
      BKK_SCREEN_HELPER_TEXT_MAX_LEN - 1
    );
    helper_data.helper_text[i][BKK_SCREEN_HELPER_TEXT_MAX_LEN - 1] = '\0'; // Ensure null-termination

    strncpy(
      helper_data.qr_code_data[i], 
      qr_command[i].c_str(), 
      BKK_SCREEN_QR_CODE_MAX_LEN - 1
    );
    helper_data.qr_code_data[i][BKK_SCREEN_QR_CODE_MAX_LEN - 1] = '\0'; // Ensure null-termination
  }

  const bkk_screen_error_code_t set_res 
    = bkk_screen_client_set_helper_screen_data(
      key, &helper_data);

  if (set_res != BKK_SCREEN_ERROR_NONE) {
    log_error("Update", (
      "Failed to set helper screen data, error code: " 
      + std::to_string(set_res)).c_str());
    return -1;
  }
  return 0;
}


int send_arrival_info(std::vector<arrival_info_t> & arrivals) {
  if(main_context_state != context_state::DISPLAY_ARRIVAL) {
    log_error("Update", 
      "Cannot send arrival info when not in DISPLAY_ARRIVAL state");
    return -1;
  }

  int key;
  bkk_screen_component_id_t component_id;

  const int res = get_screen_content_ctx(
    &screen_content_ctx, 
    &component_id, &key);
  
  if(res < 0) {
    log_error("Update", "Failed to get main content context");
    return -1;
  }

  const int set_res = bkk_screen_client_set_table_data(
    key, arrivals);
  if (set_res != BKK_SCREEN_ERROR_NONE) {
    log_error("Update", (
      "Failed to set table data, error code: "
      + std::to_string(set_res)).c_str());
    return -1;
  }
  return 0;

}


// ----------------------------------------------------------------------------
// Local helper function implementations: 
// ----------------------------------------------------------------------------

// protected setting of screen component related context data (ID and key)
static int set_screen_content_ctx(
    const bkk_screen_component_id_t * const component_id, 
    const int * const component_key,
    screen_content_ctx_t * const ctx) {

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

// protected getting of screen component related context data (ID and key)
static int get_screen_content_ctx(
    screen_content_ctx_t * const ctx,
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

} // namespace screen_ctx
/*
static int send_arrival_data(void * arg) {
  screen_content_ctx_t * ctx = static_cast<screen_content_ctx_t *>(arg);

  log_info("Update", "Fetching arrival data from API and sending it to the screen");

  int component_key;
  bkk_screen_component_id_t component_id; 

  const int ctx_res = get_screen_content_ctx(ctx, 
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
*/

