#include "am_http_server.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chttp.h"
#include "cJSON.h"
#include "rbuflogd/logger.h"

/* --------------------------------------------------------------------------
 * Module state
 * -------------------------------------------------------------------------- */

/* Describes a single static file to be served */
typedef struct {
  char        www_dir[512];
  const char *filename;
  const char *content_type;
} static_file_ctx_t;

#define STATIC_FILE_COUNT 3

typedef struct {
  const app_info_list_t *app_info_list;
  chttp_server_t        *server;
  static_file_ctx_t      static_files[STATIC_FILE_COUNT];
} http_server_ctx_t;

static http_server_ctx_t  g_ctx;
static pthread_t          g_thread;

/* --------------------------------------------------------------------------
 * Static file serving
 * -------------------------------------------------------------------------- */

/* Read a file from disk into a heap buffer; sets *len_out to the byte count.
 * Returns NULL on error. Caller must free the returned buffer. */
static char *read_file_bytes(const char *path, size_t *len_out)
{
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;

  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
  const long size = ftell(f);
  if (size < 0)                    { fclose(f); return NULL; }
  rewind(f);

  char *buf = (char *)malloc((size_t)size + 1);
  if (!buf)                        { fclose(f); return NULL; }

  if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
    free(buf); fclose(f); return NULL;
  }
  buf[size] = '\0';
  fclose(f);

  *len_out = (size_t)size;
  return buf;
}

static void handle_static_file(const chttp_request_t *req,
                               chttp_response_t      *resp,
                               void                  *user_data)
{
  (void)req;
  const static_file_ctx_t *ctx = (const static_file_ctx_t *)user_data;

  char path[640];
  snprintf(path, sizeof(path), "%s/%s", ctx->www_dir, ctx->filename);

  size_t len = 0;
  char  *body = read_file_bytes(path, &len);
  if (!body) {
    resp->status       = "404 Not Found";
    resp->content_type = "text/plain";
    resp->body         = strdup("not found");
    resp->body_len     = 9;
    return;
  }

  resp->status       = "200 OK";
  resp->content_type = ctx->content_type;
  resp->body         = body;
  resp->body_len     = len;
}

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static const char *app_status_to_string(app_status_enum_t status)
{
  switch (status) {
    case APP_STATUS_NOT_STARTED:        return "NOT_STARTED";
    case APP_STATUS_NOT_IN_THIS_PHASE:  return "NOT_IN_THIS_PHASE";
    case APP_STATUS_RUNNING:            return "RUNNING";
    case APP_STATUS_EXITED:             return "EXITED";
    case APP_STATUS_KILLED:             return "KILLED";
    case APP_STATUS_FAILED:             return "FAILED";
    case APP_STATUS_OTHER_ERROR:        return "OTHER_ERROR";
    default:                            return "UNKNOWN";
  }
}

/* Build a JSON status document into a heap-allocated buffer.
 * Caller is responsible for freeing the returned pointer. */
static char *build_status_json(const app_info_list_t *list)
{
  cJSON *root = cJSON_CreateObject();
  if (!root) {
    return NULL;
  }

  cJSON *apps = cJSON_AddArrayToObject(root, "apps");
  if (!apps) {
    cJSON_Delete(root);
    return NULL;
  }

  for (int i = 0; i < list->num_apps; i++) {
    const app_info_t *app = &list->app[i];

    cJSON *entry = cJSON_CreateObject();
    if (!entry) {
      cJSON_Delete(root);
      return NULL;
    }

    cJSON_AddStringToObject(
      entry, "name",   
      (app->name != NULL) ? app->name : "n.a.");

    cJSON_AddNumberToObject(
      entry, "pid", app->pid);

    cJSON_AddStringToObject(
      entry, "status", 
      app_status_to_string(app->status));

    cJSON_AddNumberToObject(
      entry, "exit_code", app->exit_code);

    cJSON_AddItemToArray(apps, entry);
  }

  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  return json_str; /* heap-allocated; caller must free */
}

/* --------------------------------------------------------------------------
 * Route handler
 * -------------------------------------------------------------------------- */

static void handle_status(const chttp_request_t *req, 
    chttp_response_t *resp, void *user_data) {
  (void)req;
  const app_info_list_t *list = (const app_info_list_t *)user_data;

  char *json_str = build_status_json(list);
  if (!json_str) {
    const char *err = "{\"error\":\"out of memory\"}";
    resp->status       = "500 Internal Server Error";
    resp->content_type = "application/json";
    resp->body         = strdup(err);
    resp->body_len     = strlen(err);
    return;
  }

  resp->status       = "200 OK";
  resp->content_type = "application/json";
  resp->body_len     = strlen(json_str);
  resp->body         = json_str; /* chttp frees this after send */
}

/* --------------------------------------------------------------------------
 * Server thread
 * -------------------------------------------------------------------------- */

static void *http_server_thread(void *arg)
{
  http_server_ctx_t *ctx = (http_server_ctx_t *)arg;
  chttp_server_run(ctx->server);
  return NULL;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

int am_http_server_start(const app_info_list_t *app_info_list,
                         uint16_t port,
                         const char *www_dir)
{
  g_ctx.app_info_list = app_info_list;
  g_ctx.server = chttp_server_create(port);
  if (!g_ctx.server) {
    log_error("http", "failed to create HTTP server");
    return -1;
  }

  if (chttp_server_register_route(g_ctx.server, "GET", "/status",
                                  handle_status,
                                  (void *)app_info_list) != 0) {
    log_error("http", "failed to register /status route");
    chttp_server_destroy(g_ctx.server);
    g_ctx.server = NULL;
    return -1;
  }

  /* Register static file routes when a www directory is provided */
  if (www_dir) {
    static const struct { const char *route; const char *file; const char *ct; }
    static_defs[STATIC_FILE_COUNT] = {
      { "/",          "index.html", "text/html"              },
      { "/style.css", "style.css",  "text/css"               },
      { "/app.js",    "app.js",     "application/javascript" },
    };

    for (int i = 0; i < STATIC_FILE_COUNT; i++) {
      snprintf(g_ctx.static_files[i].www_dir,
               sizeof(g_ctx.static_files[i].www_dir),
               "%s", www_dir);
      g_ctx.static_files[i].filename     = static_defs[i].file;
      g_ctx.static_files[i].content_type = static_defs[i].ct;

      if (chttp_server_register_route(g_ctx.server, "GET", static_defs[i].route,
                                      handle_static_file,
                                      &g_ctx.static_files[i]) != 0) {
        log_error("http", "failed to register static file route");
        chttp_server_destroy(g_ctx.server);
        g_ctx.server = NULL;
        return -1;
      }
    }
  }

  const int rc = pthread_create(
    &g_thread, 
    NULL, 
    http_server_thread, 
    &g_ctx);

  if (rc != 0) {
    log_error("http", "failed to start HTTP server thread");
    chttp_server_destroy(g_ctx.server);
    g_ctx.server = NULL;
    return -1;
  }

  char msg[64];
  snprintf(msg, sizeof(msg), "HTTP status server listening on port %u", port);
  log_info("http", msg);
  return 0;
}

void am_http_server_stop(void)
{
  if (g_ctx.server) {
    chttp_server_stop(g_ctx.server);
    pthread_join(g_thread, NULL);
    chttp_server_destroy(g_ctx.server);
    g_ctx.server = NULL;
  }
}
