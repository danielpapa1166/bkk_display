#include "http_server_client_handler.h"
#include "http_server_resource_handler.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void client_handler(int client_fd)
{
  char buffer[2048] = { 0 };

  ssize_t read_bytes = read(client_fd, buffer, sizeof(buffer) - 1);
  if (read_bytes > 0) {
    buffer[read_bytes] = '\0';
    printf("Received from client: %s \n", buffer);
    http_server_handle_resource_request(client_fd, buffer);
  } else {
    const char *response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n"
                           "Content-Length: 12\r\nConnection: close\r\n\r\nBad Request\n";
    write(client_fd, response, strlen(response));
  }

  close(client_fd);
}