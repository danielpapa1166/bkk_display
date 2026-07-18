#include "api_context.hpp"
#include <rbuflogd/logger.h>
#include "bkk_api_client.hpp"

// ----------------------------------------------------------------------------
// Local data structures and functions for managing API fetch context
// ----------------------------------------------------------------------------

static api_fetch_context_t api_fetch_ctx = {
  .mutex = PTHREAD_MUTEX_INITIALIZER,
  .api_key = "",
  .station_id_list = {},
  .station_name_list = {}
};


// ----------------------------------------------------------------------------
// local helper function headers for managing API fetch context
// ----------------------------------------------------------------------------

static int set_api_fetch_ctx(
  const std::string * const api_key,
  const std::vector<std::string> * const station_id_list,
  const std::vector<std::string> * const station_name_list,
  api_fetch_context_t * const ctx);


// ----------------------------------------------------------------------------
// global interface functions for managing API fetch context
// ----------------------------------------------------------------------------

int init_api_context() {
  pthread_mutex_init(&api_fetch_ctx.mutex, NULL);
  return load_api_context();
}


int load_api_context() {
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


std::string get_api_key(void) {
  pthread_mutex_lock(&api_fetch_ctx.mutex);
  std::string api_key = api_fetch_ctx.api_key;
  pthread_mutex_unlock(&api_fetch_ctx.mutex);
  return api_key;
}
std::vector<std::string> get_station_id_list(void) {
  pthread_mutex_lock(&api_fetch_ctx.mutex);
  std::vector<std::string> station_id_list = api_fetch_ctx.station_id_list;
  pthread_mutex_unlock(&api_fetch_ctx.mutex);
  return station_id_list;
}


std::vector<std::string> get_station_name_list(void) {
  pthread_mutex_lock(&api_fetch_ctx.mutex);
  std::vector<std::string> station_name_list = api_fetch_ctx.station_name_list;
  pthread_mutex_unlock(&api_fetch_ctx.mutex);
  return station_name_list;
}


// ----------------------------------------------------------------------------
// local helper function implementations for managing API fetch context
// ----------------------------------------------------------------------------

// protected setting of API fetch related data (API key and station lists)
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


