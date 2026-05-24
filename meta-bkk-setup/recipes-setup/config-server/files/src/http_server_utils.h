#ifndef HTTP_SERVER_UTILS_H
#define HTTP_SERVER_UTILS_H


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <chttp.h>


// Fills a chttp_response_t with a heap-allocated copy of body.
// chttp frees resp->body after sending.

static void set_simple_response(chttp_response_t *resp,
  const char *status, const char *content_type, const char *body) {

  resp->status       = status;
  resp->content_type = content_type;
  resp->body         = strdup(body);
  resp->body_len     = resp->body != NULL ? strlen(body) : 0;
}


static const char *get_mime_type(const char *path) {
  const char *dot = strrchr(path, '.');
  if (dot == NULL) {
    return NULL;
  }

  if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) {
    return "text/html; charset=utf-8";
  }
  if (strcmp(dot, ".css") == 0) {
    return "text/css; charset=utf-8";
  }
  if (strcmp(dot, ".js") == 0 || strcmp(dot, ".mjs") == 0) {
    return "application/javascript; charset=utf-8";
  }

  return NULL;
}

static int is_safe_request_path(const char *path) {
  if (path == NULL || path[0] != '/') {
    return 0;
  }

  if (strstr(path, "..") != NULL) {
    return 0;
  }

  return 1;
}

#endif // HTTP_SERVER_UTILS_H  