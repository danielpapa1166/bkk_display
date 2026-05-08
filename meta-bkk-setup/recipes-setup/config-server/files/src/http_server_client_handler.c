#include "http_server_client_handler.h"
#include "http_server_get_handler.h"
#include "http_server_post_handler.h"
#include "http_server_logger.h"
#include <cJSON.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
  HTTP_METHOD_GET,
  HTTP_METHOD_POST,
  HTTP_METHOD_UNSUPPORTED
} http_method_t;


static char logger_category[8];


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



static http_method_t parse_http_method(const char *buffer,
                                       char *path_out, size_t path_out_len) {
  char method[8] = { 0 };
  char request_path[256] = { 0 };
  char http_version[16] = { 0 };

  int retval = sscanf(buffer,
    "%7s %255s %15s", method, request_path, http_version);

  if (retval != 3) {
    return HTTP_METHOD_UNSUPPORTED;
  }

  if (path_out != NULL && path_out_len > 0) {
    strncpy(path_out, request_path, path_out_len - 1);
    path_out[path_out_len - 1] = '\0';
  }

  if (strncmp(method, "GET", 3) == 0) {
    return HTTP_METHOD_GET;
  }
  if (strncmp(method, "POST", 4) == 0) {
    return HTTP_METHOD_POST;
  }
  return HTTP_METHOD_UNSUPPORTED;
}


void client_handler(int client_fd, server_mode_t mode)
{
  struct timespec ts_start, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_start);

  rename_logger("clt_hdl", sizeof("clt_hdl") - 1);
  snprintf(logger_category, sizeof(logger_category), "p:%d", getpid());

  char buffer[2048] = { 0 };

  ssize_t read_bytes = read(client_fd, buffer, sizeof(buffer) - 1);

  if(read_bytes <= 0) {
    const char *response = 
      "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n"
      "Content-Length: 12\r\nConnection: close\r\n\r\nBad Request\n";
    write(client_fd, response, strlen(response));
    close(client_fd); 

    log_warning(
      logger_category, 
      "HTTP/1.1 400 Bad Request (read_bytes <= 0)");
    return; 
  }

  buffer[read_bytes] = '\0';
  //printf("Received from client: %s \n", buffer);

  char request_path[256] = { 0 };
  http_method_t method_type = parse_http_method(buffer, request_path, sizeof(request_path));
  int result = -1;
  if(method_type == HTTP_METHOD_POST) {
    char *response_buf = NULL;
    size_t response_len = 0;
    result = http_server_handle_post(
      buffer,
      &response_buf,
      &response_len,
      mode);

    if (result == 0) {
      write_all(client_fd, response_buf, response_len);
    }

    if(response_buf != NULL){
      free(response_buf);
    }
  }
  else if(method_type == HTTP_METHOD_GET) {
    char *response_buf = NULL;
    size_t response_len = 0;

    /* Strip query string for routing check */
    char path_no_query[256] = { 0 };
    size_t plen = strcspn(request_path, "?");
    if (plen >= sizeof(path_no_query)) plen = sizeof(path_no_query) - 1;
    memcpy(path_no_query, request_path, plen);

    if (strncmp(path_no_query, "/api/", 5) == 0) {
      result = http_server_handle_get_api(
        path_no_query,
        mode,
        &response_buf,
        &response_len);
    } else {
      result = http_server_handle_resource_request(
        buffer,
        &response_buf,
        &response_len);
    }

    if (result == 0) {
      write_all(client_fd, response_buf, response_len);
    }

    if(response_buf != NULL){
      free(response_buf);
    }
  }
  else {
    const char *response = 
      "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n"
      "Content-Length: 12\r\nConnection: close\r\n\r\nBad Request\n";
    write(client_fd, response, strlen(response));
    log_warning(logger_category, 
      "HTTP/1.1 400 Bad Request (unsupported method)");
  }

  clock_gettime(CLOCK_MONOTONIC, &ts_end);

  long elapsed_us = (ts_end.tv_sec - ts_start.tv_sec) * 1000000L +
                    (ts_end.tv_nsec - ts_start.tv_nsec) / 1000L;
  char rt_msg[128] = { 0 };
  snprintf(rt_msg, sizeof(rt_msg), 
    "%s request handled with results %d (%ld us)", 
    method_type == HTTP_METHOD_GET ? "GET" : "POST", result, elapsed_us);
  log_debug(logger_category, rt_msg);

  close(client_fd);
  cleanup_logger(); // child cleans up its own logger instance before exiting
  return; 
}