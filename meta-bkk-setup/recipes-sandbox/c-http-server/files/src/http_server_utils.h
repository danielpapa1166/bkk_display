#ifndef HTTP_SERVER_UTILS_H
#define HTTP_SERVER_UTILS_H


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>


// malloced buffer with the full HTTP response. Caller must free(*out_buf).

static int build_simple_response(char **out_buf, size_t *out_len,
  const char *status, const char *content_type, const char *body) {

  char header[512] = { 0 };
  size_t body_len = strlen(body);
  int header_len = snprintf(
    header,
    sizeof(header),
    "HTTP/1.1 %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n"
    "Connection: close\r\n\r\n",
    status, content_type, body_len);

  if (header_len <= 0) {
    return -1;
  }

  size_t total = (size_t)header_len + body_len;
  char *buf = (char*)malloc(total);
  if (buf == NULL) {
    return -1;
  }

  memcpy(buf, header, (size_t)header_len);
  memcpy(buf + header_len, body, body_len);

  *out_buf = buf;
  *out_len = total;
  return 0;
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