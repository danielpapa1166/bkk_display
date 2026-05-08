#include "http_server_resource_handler.h"
#include "http_server_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define HTTP_SERVER_STATIC_ROOT "/usr/share/c-http-server/www"


int http_server_handle_resource_request(const char *request_text,
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

  if (!is_safe_request_path(request_path)) {
    return build_simple_response(out_buf, out_len,
      "403 Forbidden",
      "text/plain; charset=utf-8",
      "Forbidden\n");
  }

  char path_without_query[256] = { 0 };
  size_t copy_len = strcspn(request_path, "?");
  if (copy_len >= sizeof(path_without_query)) {
    return build_simple_response(out_buf, out_len,
      "414 URI Too Long",
      "text/plain; charset=utf-8",
      "URI Too Long\n");
  }
  memcpy(path_without_query, request_path, copy_len);
  path_without_query[copy_len] = '\0';

  if (strcmp(path_without_query, "/") == 0) {
    strncpy(path_without_query, "/index.html", sizeof(path_without_query) - 1);
  }

  for (size_t i = 0; path_without_query[i] != '\0'; i++) {
    if ((unsigned char)path_without_query[i] < 32) {
      return build_simple_response(out_buf, out_len,
        "400 Bad Request",
        "text/plain; charset=utf-8",
        "Bad Request\n");
    }
  }

  char full_path[512] = { 0 };

  int snprintf_res = snprintf(
    full_path,
    sizeof(full_path),
    "%s/%s",
    HTTP_SERVER_STATIC_ROOT,
    (path_without_query[0] == '/') ? path_without_query + 1 : path_without_query);

  if (snprintf_res >= (int)sizeof(full_path)) {
    return build_simple_response(out_buf, out_len,
      "414 URI Too Long",
      "text/plain; charset=utf-8",
      "URI Too Long\n");
  }

  const char *mime_type = get_mime_type(full_path);
  if (mime_type == NULL) {
    return build_simple_response(out_buf, out_len,
      "415 Unsupported Media Type",
      "text/plain; charset=utf-8",
      "Only .html, .css, and .js files are served\n");
  }

  struct stat file_stat = { 0 };
  if (stat(full_path, &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
    return build_simple_response(out_buf, out_len,
      "404 Not Found",
      "text/plain; charset=utf-8",
      "Not Found\n");
  }

  FILE *file = fopen(full_path, "rb");
  if (file == NULL) {
    return build_simple_response(out_buf, out_len,
      "500 Internal Server Error",
      "text/plain; charset=utf-8",
      "Internal Server Error\n");
  }

  char header[512] = { 0 };
  int header_len = snprintf(
    header,
    sizeof(header),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %ld\r\n"
    "Connection: close\r\n\r\n",
    mime_type, (long)file_stat.st_size);

  if (header_len <= 0) {
    fclose(file);
    return -1;
  }

  size_t file_size = (size_t)file_stat.st_size;
  size_t total = (size_t)header_len + file_size;
  char *buf = malloc(total);
  if (buf == NULL) {
    fclose(file);
    return -1;
  }

  memcpy(buf, header, (size_t)header_len);
  size_t read_total = fread(buf + header_len, 1, file_size, file);
  fclose(file);

  if (read_total != file_size) {
    free(buf);
    return -1;
  }

  *out_buf = buf;
  *out_len = total;
  return 0;
}