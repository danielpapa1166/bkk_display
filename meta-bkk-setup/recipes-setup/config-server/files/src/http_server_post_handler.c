#include "http_server_post_handler.h"
#include "cJSON.h"
#include "http_server_utils.h"
#include "http_server_config.h"
#include "http_server_user_action_handler.h"
#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------------------
// local function declarations
// ----------------------------------------------------------------------------

static int parse_api_button_request(const cJSON *json, api_button_request_t *out_request);

// ----------------------------------------------------------------------------
// public function implementations
// ----------------------------------------------------------------------------

void http_server_handle_button_post(const chttp_request_t *req,
                                    chttp_response_t *resp,
                                    void *user_data) {
  server_mode_t mode = *(server_mode_t *)user_data;

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

void http_server_handle_finish_post(const chttp_request_t *req,
                                    chttp_response_t *resp,
                                    void *user_data) {
  (void)req;
  (void)user_data;
  printf("Received /api/finish POST request\n");
  set_simple_response(resp, "200 OK", "text/plain; charset=utf-8", "ok\n");
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
