#include "http_server_post_handler.h"
#include "cJSON.h"
#include "http_server_utils.h"
#include <stdio.h>

// ----------------------------------------------------------------------------
// local types
// ----------------------------------------------------------------------------

typedef struct {
  char action[16];
  char from_page[64];
  char to_page[64];
} api_button_request_t;


typedef int (http_post_handler_fn)          \
    (const char *request_text,              \
      api_button_request_t *out_request); 

typedef struct {
  char request_path[256];
  http_post_handler_fn *handler_fn; 
} post_handler_table_entry_t;

// ----------------------------------------------------------------------------
// local function declarations
// ----------------------------------------------------------------------------

static int handle_button_api_post(const char *request_text, 
  api_button_request_t *out_request);
static int extract_http_json_body(const char *request_text,
    char *out_json, size_t out_json_size);
static int parse_api_button_request(const cJSON *json, api_button_request_t *out_request);

// ----------------------------------------------------------------------------
// local variables
// ----------------------------------------------------------------------------

static post_handler_table_entry_t post_handler_table[] = {
    { .request_path = "/api/button", 
          .handler_fn = (http_post_handler_fn *)&handle_button_api_post 
        }, 
}; 


// ----------------------------------------------------------------------------
// public function implementations
// ----------------------------------------------------------------------------

int http_server_handle_post(const char *request_text,
    char **out_buf, size_t *out_len) {

  char method[8] = { 0 };
  char request_path[256] = { 0 };
  char http_version[16] = { 0 };

  if (sscanf(request_text, "%7s %255s %15s", method, request_path, http_version) != 3) {
    return build_simple_response(out_buf, out_len,
      "400 Bad Request",
      "text/plain; charset=utf-8",
      "Bad Request\n");
  }

  printf("Handling POST request for path: %s\n", request_path);

  for (size_t i = 0; i < sizeof(post_handler_table) / sizeof(post_handler_table_entry_t); i++) {
    if (strcmp(request_path, post_handler_table[i].request_path) == 0) {
      api_button_request_t request = {0};
      int handler_result = post_handler_table[i].handler_fn(request_text, &request);
      if (handler_result != 0) {
        return build_simple_response(out_buf, out_len,
          "500 Internal Server Error", 
          "text/plain; charset=utf-8",         
          "Internal Server Error\n");
      }
       return build_simple_response(out_buf, out_len,
        "200 OK",
        "text/plain; charset=utf-8",
        "ok\n");
    }
  }

  return build_simple_response(out_buf, out_len,
    "200 OK",
    "text/plain; charset=utf-8",
    "ok\n");
}


// ----------------------------------------------------------------------------
// local function implementations
// ----------------------------------------------------------------------------

static int extract_http_json_body(const char *request_text,
    char *out_json, size_t out_json_size) {
  const char *body_start = strstr(request_text, "\r\n\r\n");
  size_t body_len = 0;

  if (body_start != NULL) {
    body_start += 4;
  } else {
    body_start = strstr(request_text, "\n\n");
    if (body_start != NULL) {
      body_start += 2;
    }
  }

  if (body_start == NULL || *body_start == '\0') {
    return -1;
  }

  while (*body_start == '\r' || *body_start == '\n') {
    body_start++;
  }

  body_len = strlen(body_start);
  while (body_len > 0 &&
      (body_start[body_len - 1] == '\r' || body_start[body_len - 1] == '\n')) {
    body_len--;
  }

  if (body_len == 0 || body_len >= out_json_size) {
    return -1;
  }

  memcpy(out_json, body_start, body_len);
  out_json[body_len] = '\0';
  return 0;
}


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
  return 0;
}

static int handle_button_api_post(
    const char *request_text, api_button_request_t *out_request) {
  
  char json_body[2048] = { 0 };
  const int retval = extract_http_json_body(
    request_text, 
    json_body, 
    sizeof(json_body));

  if (retval == 0) {
    printf("Extracted JSON body: %s\n", json_body);
    cJSON *json = cJSON_Parse(json_body);
    if (json == NULL) {
      printf("Failed to parse JSON body\n");
      return -1;
    }

    int parse_result = parse_api_button_request(json, out_request);
    cJSON_Delete(json);
    if (parse_result != 0) {
      return -1;
    }

    printf("Button request: action='%s' from='%s' to='%s'\n",
        out_request->action, out_request->from_page, out_request->to_page);
  } 
  else {
    printf("No JSON body found in POST request\n");
    return -1;
  }
  return 0;
}