#include "http_server_post_handler.h"
#include "cJSON.h"
#include "http_server_utils.h"
#include <network_manager_pub.h>
#include "http_server_user_action_handler.h"
#include "bkk_stop_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbuflogd/logger.h"

#define STATION_SEARCH_MAX_QUERY_LEN 64
#define STATION_SEARCH_MIN_QUERY_LEN 2
#define STATION_SEARCH_MAX_RESULTS   25

// ----------------------------------------------------------------------------
// local function declarations
// ----------------------------------------------------------------------------

static int parse_api_button_request(const cJSON *json, api_button_request_t *out_request);

// ----------------------------------------------------------------------------
// public function implementations
// ----------------------------------------------------------------------------

void http_server_handle_button_post(
    const chttp_request_t *req,
    chttp_response_t *resp,
    void *user_data) {
  
  log_debug("btn_post", "Handling button post"); 
  network_manager_mode_t mode = *(network_manager_mode_t *)user_data;

  if (req->body == NULL || req->body_len == 0) {
    printf("No JSON body found in POST request\n");
    set_simple_response(resp, "400 Bad Request",
      "text/plain; charset=utf-8", "Bad Request\n");
    return;
  }

  cJSON *json = cJSON_ParseWithLength(req->body, req->body_len);
  if (json == NULL) {
    printf("Failed to parse JSON body\n");
    set_simple_response(resp, "400 Bad Request",
      "text/plain; charset=utf-8", "Bad Request\n");
    return;
  }

  api_button_request_t request = { 0 };
  int parse_result = parse_api_button_request(json, &request);
  cJSON_Delete(json);

  if (parse_result != 0) {
    set_simple_response(resp, "400 Bad Request",
      "text/plain; charset=utf-8", "Bad Request\n");
    return;
  }

  printf("Button request: action='%s' from='%s' to='%s'\n",
      request.action, request.from_page, request.to_page);

  if (handle_user_action(&request, mode) != 0) {
    printf("handle_user_action failed\n");
    set_simple_response(resp, "500 Internal Server Error",
      "text/plain; charset=utf-8", "Internal Server Error\n");
    return;
  }

  set_simple_response(resp, "200 OK", "text/plain; charset=utf-8", "ok\n");
}

void http_server_handle_finish_post(
    const chttp_request_t *req,
    chttp_response_t *resp,
    void *user_data) {

  log_debug("fnsh_hdl", "Handling finish post"); 
  (void)req;
  (void)user_data;
  printf("Received /api/finish POST request\n");
  set_simple_response(resp, "200 OK", "text/plain; charset=utf-8", "ok\n");

  log_debug("fnsh_hdl", "Finish post handled successfully"); 
}

void http_server_handle_station_search_post(
    const chttp_request_t *req,
    chttp_response_t *resp,
    void *user_data) {

  log_debug("stn_srch", "Handling station search post");
  (void)user_data;

  if (req->body == NULL || req->body_len == 0) {
    set_simple_response(resp, "400 Bad Request",
      "text/plain; charset=utf-8", "Bad Request\n");
    return;
  }

  cJSON *json = cJSON_ParseWithLength(req->body, req->body_len);
  if (json == NULL) {
    set_simple_response(resp, "400 Bad Request",
      "text/plain; charset=utf-8", "Bad Request\n");
    return;
  }

  const cJSON *query_item = cJSON_GetObjectItemCaseSensitive(json, "query");
  if (!cJSON_IsString(query_item) || query_item->valuestring == NULL) {
    cJSON_Delete(json);
    set_simple_response(resp, "400 Bad Request",
      "text/plain; charset=utf-8", "Missing 'query' field\n");
    return;
  }

  char query[STATION_SEARCH_MAX_QUERY_LEN + 1];
  snprintf(query, sizeof(query), "%s", query_item->valuestring);
  cJSON_Delete(json);

  if (strlen(query) < STATION_SEARCH_MIN_QUERY_LEN) {
    set_simple_response(resp, "400 Bad Request",
      "text/plain; charset=utf-8", "Query too short\n");
    log_warning("stn_srch", "400 Bad Request: query too short");
    return;
  }

  size_t *indices = NULL;
  size_t count = 0;
  bkk_stop_stat_t stat = find_stops_by_name_substring(query, &indices, &count);

  cJSON *root = cJSON_CreateObject();
  cJSON *stations = cJSON_AddArrayToObject(root, "stations");

  if (stat == BKK_STOP_FOUND) {
    size_t result_count = count < STATION_SEARCH_MAX_RESULTS ? count : STATION_SEARCH_MAX_RESULTS;
    for (size_t i = 0; i < result_count; i++) {
      bkk_stop_t stop;
      if (find_stop_by_index(indices[i], &stop) == BKK_STOP_FOUND) {
        cJSON *station_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(station_obj, "stop_id", stop.stop_id);
        cJSON_AddStringToObject(station_obj, "stop_name", stop.stop_name);
        cJSON_AddItemToArray(stations, station_obj);
      }
    }
    cJSON_AddNumberToObject(root, "count", (double)count);
  } else {
    cJSON_AddNumberToObject(root, "count", 0);
  }

  free(indices);

  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  if (json_str == NULL) {
    set_simple_response(resp, "500 Internal Server Error",
      "text/plain; charset=utf-8", "Internal Server Error\n");
    return;
  }

  resp->status       = "200 OK";
  resp->content_type = "application/json; charset=utf-8";
  resp->body         = json_str; /* chttp frees this after send */
  resp->body_len     = strlen(json_str);

  log_debug("stn_srch", "Station search handled successfully");
}

// ----------------------------------------------------------------------------
// local function implementations
// ----------------------------------------------------------------------------

static int parse_api_button_request(const cJSON *json, api_button_request_t *out_request) {
  const cJSON *action    = cJSON_GetObjectItemCaseSensitive(json, "action");
  const cJSON *from_page = cJSON_GetObjectItemCaseSensitive(json, "from_page");
  const cJSON *to_page   = cJSON_GetObjectItemCaseSensitive(json, "to_page");

  if (!cJSON_IsString(action) || action->valuestring == NULL) {
    printf("parse_api_button_request: missing or invalid 'action'\n");
    return -1;
  }
  if (!cJSON_IsString(from_page) || from_page->valuestring == NULL) {
    printf("parse_api_button_request: missing or invalid 'from_page'\n");
    return -1;
  }
  if (!cJSON_IsString(to_page) || to_page->valuestring == NULL) {
    printf("parse_api_button_request: missing or invalid 'to_page'\n");
    return -1;
  }

  snprintf(out_request->action,    sizeof(out_request->action),    "%s", action->valuestring);
  snprintf(out_request->from_page, sizeof(out_request->from_page), "%s", from_page->valuestring);
  snprintf(out_request->to_page,   sizeof(out_request->to_page),   "%s", to_page->valuestring);

  const cJSON *wifi_ssid     = cJSON_GetObjectItemCaseSensitive(json, "wifi_ssid");
  const cJSON *wifi_password = cJSON_GetObjectItemCaseSensitive(json, "wifi_password");
  const cJSON *api_key       = cJSON_GetObjectItemCaseSensitive(json, "api_key");
  const cJSON *station_ids   = cJSON_GetObjectItemCaseSensitive(json, "station_ids");

  if (cJSON_IsString(wifi_ssid) && wifi_ssid->valuestring)
    snprintf(out_request->wifi_ssid, sizeof(out_request->wifi_ssid), "%s", wifi_ssid->valuestring);
  if (cJSON_IsString(wifi_password) && wifi_password->valuestring)
    snprintf(out_request->wifi_password, sizeof(out_request->wifi_password), "%s", wifi_password->valuestring);
  if (cJSON_IsString(api_key) && api_key->valuestring)
    snprintf(out_request->api_key, sizeof(out_request->api_key), "%s", api_key->valuestring);
  if (cJSON_IsString(station_ids) && station_ids->valuestring)
    snprintf(out_request->station_ids, sizeof(out_request->station_ids), "%s", station_ids->valuestring);

  return 0;
}
