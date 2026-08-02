#include "http_server_get_handler.h"
#include "http_server_utils.h"
#include <network_manager_pub.h>
#include "rbuflogd/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define HTTP_SERVER_STATIC_ROOT "/usr/share/config-server/www"


void http_server_handle_resource_request(
    const chttp_request_t *req,
    chttp_response_t *resp,
    void *user_data) {

  log_debug("rsc_hdl", "Handling resource request.");               
  (void)user_data;

  const char *path = req->path;

  if (!is_safe_request_path(path)) {
    set_simple_response(resp, "403 Forbidden",
      "text/plain; charset=utf-8", "Forbidden\n");

    log_warning("rsc_hdl", "403 Forbidden"); 
    return;
  }

  char path_clean[256] = { 0 };
  if (strcmp(path, "/") == 0) {
    strncpy(path_clean, "/index.html", sizeof(path_clean) - 1);
  } 
  else {
    strncpy(path_clean, path, sizeof(path_clean) - 1);
  }

  for (size_t i = 0; path_clean[i] != '\0'; i++) {
    if ((unsigned char)path_clean[i] < 32) {
      set_simple_response(resp, "400 Bad Request",
        "text/plain; charset=utf-8", "Bad Request\n");
      log_warning("rsc_hdl", "400 Bad Request"); 
      return;
    }
  }

  char full_path[512] = { 0 };
  int snprintf_res = snprintf(
    full_path, sizeof(full_path), "%s/%s",
    HTTP_SERVER_STATIC_ROOT,
    (path_clean[0] == '/') ? path_clean + 1 : path_clean);

  if (snprintf_res >= (int)sizeof(full_path)) {
    set_simple_response(resp, "414 URI Too Long",
      "text/plain; charset=utf-8", "URI Too Long\n");

    log_warning("rsc_hdl", "414 URI Too Long"); 
    return;
  }

  const char *mime_type = get_mime_type(full_path);
  if (mime_type == NULL) {
    set_simple_response(resp, "415 Unsupported Media Type",
      "text/plain; charset=utf-8",
      "Only .html, .css, and .js files are served\n");

    log_warning("rsc_hdl", "415 Unsupported Media Type"); 
    return;
  }

  struct stat file_stat = { 0 };
  if (stat(full_path, &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
    set_simple_response(resp, "404 Not Found",
      "text/plain; charset=utf-8", "Not Found\n");
    
    log_warning("rsc_hdl", "404 Not Found"); 
    return;
  }

  FILE *file = fopen(full_path, "rb");
  if (file == NULL) {
    set_simple_response(resp, "500 Internal Server Error",
      "text/plain; charset=utf-8", "Internal Server Error\n");
    
    log_warning("rsc_hdl", "500 Internal Server Error"); 
    return;
  }

  size_t file_size = (size_t)file_stat.st_size;
  char *buf = malloc(file_size);
  if (buf == NULL) {
    fclose(file);
    set_simple_response(resp, "500 Internal Server Error",
      "text/plain; charset=utf-8", "Internal Server Error\n");
    
    log_warning("rsc_hdl", "500 Internal Server Error"); 
    return;
  }

  size_t read_total = fread(buf, 1, file_size, file);
  fclose(file);

  if (read_total != file_size) {
    free(buf);
    set_simple_response(resp, "500 Internal Server Error",
      "text/plain; charset=utf-8", "Internal Server Error\n");

    log_warning("rsc_hdl", "500 Internal Server Error"); 
    return;
  }

  resp->status       = "200 OK";
  resp->content_type = mime_type;
  resp->body         = buf;
  resp->body_len     = file_size;

  log_debug("rsc_hdl", "Resource request handled successfully."); 
}

void http_server_handle_get_api(
    const chttp_request_t *req,
    chttp_response_t *resp,
    void *user_data) {

  log_debug("api_hdl", "Handling get api request"); 
  network_manager_mode_t mode = *(network_manager_mode_t *)user_data;

  if (strcmp(req->path, "/api/mode") == 0) {
    const char *body = 
      (mode == NETWORK_MANAGER_MODE_ACCESS_POINT) ? "{\"mode\":\"access_point\"}\n" : 
      (mode == NETWORK_MANAGER_MODE_WIFI_CLIENT) ? "{\"mode\":\"wifi_client\"}\n" : 
      "{\"mode\":\"unknown\"}\n";
    set_simple_response(resp, "200 OK", "application/json; charset=utf-8", body);
    log_debug("api_hdl", "Served API mode successfully"); 
    return;
  }

  set_simple_response(resp, "404 Not Found",
    "text/plain; charset=utf-8", "Not Found\n");

  log_warning("api_hdl", "404 Not Found"); 
}