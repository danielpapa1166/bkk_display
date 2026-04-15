#include "http_server_resource_handler.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define HTTP_SERVER_STATIC_ROOT "/usr/share/c-http-server/www"

static int write_all(int fd, const char *data, size_t len) {
  size_t written = 0;

  while (written < len) {
    ssize_t ret = write(fd, data + written, len - written);
    if (ret <= 0) {
      return -1;
    }
    written += (size_t)ret;
  }

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

  return NULL;
}

static void send_simple_response(int client_fd, 
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
    return;
  }

  if (write_all(client_fd, header, (size_t)header_len) < 0) {
    return;
  }
  write_all(client_fd, body, body_len);
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

void http_server_handle_resource_request(int client_fd, const char *request_text) {
  char method[8] = { 0 };
  char request_path[256] = { 0 };
  char http_version[16] = { 0 };

  if (sscanf(request_text, "%7s %255s %15s", method, request_path, http_version) != 3) {
    send_simple_response(client_fd, 
      "400 Bad Request", 
      "text/plain; charset=utf-8", 
      "Bad Request\n");
    return;
  }

  if (strcmp(method, "GET") != 0) {
    send_simple_response(client_fd, 
      "405 Method Not Allowed", 
      "text/plain; charset=utf-8",
      "Only GET is supported\n");
    return;
  }

  if (!is_safe_request_path(request_path)) {
    send_simple_response(client_fd, 
      "403 Forbidden", 
      "text/plain; charset=utf-8", 
      "Forbidden\n");
    return;
  }

  char path_without_query[256] = { 0 };
  size_t copy_len = strcspn(request_path, "?");
  if (copy_len >= sizeof(path_without_query)) {
    send_simple_response(client_fd, 
      "414 URI Too Long", 
      "text/plain; charset=utf-8", 
      "URI Too Long\n");
    return;
  }
  memcpy(path_without_query, request_path, copy_len);
  path_without_query[copy_len] = '\0';

  if (strcmp(path_without_query, "/") == 0) {
    strncpy(path_without_query, "/index.html", sizeof(path_without_query) - 1);
  }

  for (size_t i = 0; path_without_query[i] != '\0'; i++) {
    if ((unsigned char)path_without_query[i] < 32) {
      send_simple_response(client_fd, 
        "400 Bad Request", 
        "text/plain; charset=utf-8", 
        "Bad Request\n");
      return;
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
    send_simple_response(client_fd, 
      "414 URI Too Long", 
      "text/plain; charset=utf-8", 
      "URI Too Long\n");
    return;
  }

  const char *mime_type = get_mime_type(full_path);
  if (mime_type == NULL) {
    send_simple_response(client_fd, 
      "415 Unsupported Media Type", 
      "text/plain; charset=utf-8",
      "Only .html and .css files are served\n");
    return;
  }

  struct stat file_stat = { 0 };
  if (stat(full_path, &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
    send_simple_response(client_fd, 
      "404 Not Found", 
      "text/plain; charset=utf-8", 
      "Not Found\n");
    return;
  }

  FILE *file = fopen(full_path, "rb");
  if (file == NULL) {
    send_simple_response(client_fd, 
      "500 Internal Server Error", 
      "text/plain; charset=utf-8",
      "Internal Server Error\n");
    return;
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

  if (header_len <= 0 || write_all(client_fd, header, (size_t)header_len) < 0) {
    fclose(file);
    return;
  }

  char chunk[4096] = { 0 };
  while (!feof(file)) {
    size_t n = fread(chunk, 1, sizeof(chunk), file);
    if (n > 0 && write_all(client_fd, chunk, n) < 0) {
      break;
    }
    if (ferror(file)) {
      break;
    }
  }

  fclose(file);
}