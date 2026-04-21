#include "http_server_client_handler.h"
#include "http_server_resource_handler.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef enum {
  HTTP_METHOD_GET,
  HTTP_METHOD_POST,
  HTTP_METHOD_UNSUPPORTED
} http_method_t;

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


void client_handler(int client_fd)
{
  char buffer[2048] = { 0 };

  ssize_t read_bytes = read(client_fd, buffer, sizeof(buffer) - 1);

  if(read_bytes <= 0) {
    const char *response = 
      "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n"
      "Content-Length: 12\r\nConnection: close\r\n\r\nBad Request\n";
    write(client_fd, response, strlen(response));
    close(client_fd); 
    return; 
  }

  buffer[read_bytes] = '\0';
  printf("Received from client: %s \n", buffer);

  http_method_t method_type = parse_http_method(buffer); 

  if(method_type == HTTP_METHOD_POST) {
    const char *response = 
      "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/plain\r\n"
      "Content-Length: 18\r\nConnection: close\r\n\r\nMethod Not Allowed\n";
    write(client_fd, response, strlen(response));
  }
  else if(method_type == HTTP_METHOD_GET) {
    char *response_buf = NULL;
    size_t response_len = 0;
    int result = http_server_handle_resource_request(
      buffer, 
      &response_buf, 
      &response_len);

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
  }

  close(client_fd);
  return; 
}