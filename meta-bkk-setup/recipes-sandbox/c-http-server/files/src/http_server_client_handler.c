#include "http_server_client_handler.h"
#include "http_server_resource_handler.h"
#include "http_server_post_handler.h"
#include "rbuflogd/pub_common_types.h"
#include <rbuflogd/producer.h>
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


static rbuflogd_producer_t *global_logger_producer = NULL;
static const char logger_name[] = "clt_hdl";
static char logger_category[RBUF_PROD_ID_MAX_LEN] = { 0 };


static void configure_logger(rbuflogd_producer_t *producer) {
  if(producer == NULL) {
    return;
  }

  global_logger_producer = producer;

  memcpy(
    global_logger_producer->producer_name, 
    logger_name, 
    sizeof(logger_name)); 
  
  snprintf(
    logger_category, 
    RBUF_PROD_ID_MAX_LEN, 
    "%d", getpid());
}

static int rbuflog(rbuflogd_log_level_t level, const char * message) {
  if(global_logger_producer == NULL) {
    return -1;
  }

  const int log_res = rbuflogd_producer_log(
    global_logger_producer, 
    level, 
    logger_category, 
    message);

  return log_res;
}


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



static http_method_t parse_http_method(const char *buffer) {
  char method[8] = { 0 };
  char request_path[256] = { 0 };
  char http_version[16] = { 0 };

  int retval = sscanf(buffer, 
    "%7s %255s %15s", method, request_path, http_version);

  if (retval != 3) {
    return HTTP_METHOD_UNSUPPORTED;
  }
   
  if (strncmp(method, "GET", 3) == 0) {
    return HTTP_METHOD_GET;
  }
  if (strncmp(method, "POST", 4) == 0) {
    return HTTP_METHOD_POST;
  }
  return HTTP_METHOD_UNSUPPORTED;
}


void client_handler(int client_fd, rbuflogd_producer_t *logger_producer)
{
  struct timespec ts_start, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_start);

  configure_logger(logger_producer);

  char buffer[2048] = { 0 };

  ssize_t read_bytes = read(client_fd, buffer, sizeof(buffer) - 1);

  if(read_bytes <= 0) {
    const char *response = 
      "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n"
      "Content-Length: 12\r\nConnection: close\r\n\r\nBad Request\n";
    write(client_fd, response, strlen(response));
    close(client_fd); 

    rbuflog(RBUF_LOG_LEVEL_WARNING, 
      "HTTP/1.1 400 Bad Request (read_bytes <= 0)");
    return; 
  }

  buffer[read_bytes] = '\0';
  //printf("Received from client: %s \n", buffer);

  http_method_t method_type = parse_http_method(buffer); 

  if(method_type == HTTP_METHOD_POST) {
    char *response_buf = NULL;
    size_t response_len = 0;
    int result = http_server_handle_post(
      buffer, 
      &response_buf, 
      &response_len);

    char log_msg[256] = { 0 };
    snprintf(log_msg, sizeof(log_msg), 
    "POST request handled with result: %d", result);
    rbuflog(RBUF_LOG_LEVEL_INFO, log_msg);

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
    int result = http_server_handle_resource_request(
      buffer, 
      &response_buf, 
      &response_len);

    char log_msg[256] = { 0 };
    snprintf(log_msg, sizeof(log_msg), 
    "GET request handled with result: %d", result);
    rbuflog(RBUF_LOG_LEVEL_INFO, log_msg);

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
    rbuflog(RBUF_LOG_LEVEL_WARNING, 
      "HTTP/1.1 400 Bad Request (unsupported method)");
  }

  clock_gettime(CLOCK_MONOTONIC, &ts_end);

  long elapsed_us = (ts_end.tv_sec - ts_start.tv_sec) * 1000000L +
                    (ts_end.tv_nsec - ts_start.tv_nsec) / 1000L;
  char rt_msg[64] = { 0 };
  snprintf(rt_msg, sizeof(rt_msg), "client_handler runtime: %ld us", elapsed_us);
  rbuflog(RBUF_LOG_LEVEL_DEBUG, rt_msg);

  close(client_fd);
  return; 
}