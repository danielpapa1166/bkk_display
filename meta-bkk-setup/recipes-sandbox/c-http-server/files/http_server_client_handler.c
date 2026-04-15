#include "http_server_client_handler.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

static char buffer[1024] = {0};

void client_handler(int client_fd)
{
  ssize_t read_bytes = read(client_fd, buffer, sizeof(buffer) - 1);
  if (read_bytes > 0) {
    buffer[read_bytes] = '\0';
    printf("Received from client: %s \n", buffer);
  }

  // send back a simple HTTP response
  const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, World!\n";
  write(client_fd, response, strlen(response));

  close(client_fd);
}