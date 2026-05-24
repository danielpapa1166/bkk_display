#include <stdio.h>
#include <string.h>
#include <chttp.h>
#include "http_server_config.h"
#include "http_server_get_handler.h"
#include "http_server_post_handler.h"
#include "rbuflogd/logger.h"


#define PORT 8080

/* -------------------------------------------------------------------------
 * Argument parsing
 * ------------------------------------------------------------------------- */
static int parse_args(int argc, char *argv[], server_mode_t *out_mode)
{
  const char *mode_prefix = "--mode=";
  const size_t mode_prefix_len = 7; /* strlen("--mode=") */

  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], mode_prefix, mode_prefix_len) == 0) {
      const char *value = argv[i] + mode_prefix_len;
      if (strcmp(value, "wifi") == 0) {
        *out_mode = SERVER_MODE_WIFI;
        return 0;
      }
      if (strcmp(value, "api") == 0) {
        *out_mode = SERVER_MODE_API;
        return 0;
      }
      fprintf(stderr, "Unknown --mode value: '%s' (expected 'wifi' or 'api')\n", value);
      return -1;
    }
  }

  fprintf(stderr, "Usage: %s --mode=<wifi|api>\n", argv[0]);
  return -1;
}


int main(int argc, char *argv[])
{
  rbuflogd_logger_init("http_srv");

  server_mode_t mode = SERVER_MODE_WIFI;
  if (parse_args(argc, argv, &mode) != 0) {
    log_error("main", "Failed to parse arguments. Exiting.");
    rbuflogd_logger_close();
    return 1;
  }

  const char *mode_str = (mode == SERVER_MODE_API) ? "api" : "wifi";
  printf("HTTP server starting in mode: %s\n", mode_str);
  log_info("init",
    (mode == SERVER_MODE_API)
        ? "HTTP server starting in API mode"
        : "HTTP server starting in WiFi mode");

  chttp_server_t *srv = chttp_server_create(PORT);
  if (srv == NULL) {
    log_error("main", "Failed to create HTTP server.");
    rbuflogd_logger_close();
    return 1;
  }

  chttp_server_register_route(srv, "GET",  "/api/mode",   http_server_handle_get_api,          &mode);
  chttp_server_register_route(srv, "GET",  "/",           http_server_handle_resource_request, NULL);
  chttp_server_register_route(srv, "GET",  "/index.html", http_server_handle_resource_request, NULL);
  chttp_server_register_route(srv, "GET",  "/styles.css", http_server_handle_resource_request, NULL);
  chttp_server_register_route(srv, "GET",  "/app.js",     http_server_handle_resource_request, NULL);
  chttp_server_register_route(srv, "POST", "/api/button", http_server_handle_button_post,      &mode);
  chttp_server_register_route(srv, "POST", "/api/finish", http_server_handle_finish_post,      &mode);

  log_info("main", "HTTP server running.");
  chttp_server_run(srv);

  chttp_server_destroy(srv);
  rbuflogd_logger_close();
  return 0;
}
