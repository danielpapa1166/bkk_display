#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "http_server_client_handler.h"

#define PORT 8081

static struct sockaddr_in server_addr = { 0 };


int main(void)
{
  printf("Hello world \n"); 


  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    printf("Socket error: %d \n", listen_fd);
    return 1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(PORT);

  int bind_retval = bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if(bind_retval < 0) {
    printf("Bind error: %d \n", bind_retval);
    return 1;
  }

  int listen_retval = listen(listen_fd, 5);
  if (listen_retval < 0) {
    printf("Listen error: %d \n", listen_retval);
    return 1;
  }

  int request_cnt = 0; 

  while(1) {
    int client_fd = accept(listen_fd, NULL, NULL);
    if(client_fd < 0) {
      continue;
    }

    request_cnt++;
    printf("Client connected! %d\n", request_cnt);

    int pid = fork();
    if (pid == 0) {
      printf("Child: client count %d, listen fd: %d, client fd: %d \n", 
        request_cnt, listen_fd, client_fd);
      close(listen_fd); 
      client_handler(client_fd);
      printf("Child closed for client %d\n", request_cnt); 
      return 0;
    }
    else {
      close(client_fd);
      continue;
    }
  }

  return 0;
}
