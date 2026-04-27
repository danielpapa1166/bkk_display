#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "http_server_client_handler.h"
#include "rbuflogd/pub_common_types.h"
#include <rbuflogd/producer.h>

#define PORT 8081

static struct sockaddr_in server_addr = { 0 };


int main(void)
{

  rbuflogd_producer_t producer;
  rbuflogd_producer_open(
    &producer, 
    "http_srv");

  rbuflogd_producer_log(
    &producer, 
    RBUF_LOG_LEVEL_INFO, 
    "main", 
    "HTTP server starting...");

  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    printf("Socket error %d \n", listen_fd);
    rbuflogd_producer_log(
      &producer, 
      RBUF_LOG_LEVEL_ERROR, 
      "main", 
      "Socket error");
    return 1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(PORT);

  int bind_retval = bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if(bind_retval < 0) {
    printf("Bind error %d \n", bind_retval);
    rbuflogd_producer_log(
      &producer, 
      RBUF_LOG_LEVEL_ERROR, 
      "main", 
      "Bind error");
    return 1;
  }

  int listen_retval = listen(listen_fd, 5);
  if (listen_retval < 0) {
    printf("Listen error %d \n", listen_retval);
    rbuflogd_producer_log(
      &producer, 
      RBUF_LOG_LEVEL_ERROR, 
      "main", 
      "Listen error");
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
      char child_name[RBUF_PROD_ID_MAX_LEN]; 
      char child_msg[100];
      int log_res = 0; 
      snprintf(
        child_name, 
        RBUF_PROD_ID_MAX_LEN, 
        "%d", getpid());

      strcpy(producer.producer_name, child_name); 

      snprintf(
        child_msg, 
        sizeof(child_msg), 
        "Client connected: %d (listener: %d)", client_fd, listen_fd);

      log_res = rbuflogd_producer_log(
        &producer, 
        RBUF_LOG_LEVEL_DEBUG, 
        "child", 
        child_msg);

      if(log_res < 0) {
        printf("Logging error %d \n", log_res);
      }
      else {
        printf("Logged successfully \n");
      }

      printf("Child: client count %d, listen fd: %d, client fd: %d \n", 
        request_cnt, listen_fd, client_fd);
      close(listen_fd); 
      client_handler(client_fd);
      printf("Child closed for client %d\n", request_cnt); 

      snprintf(
        child_msg, 
        sizeof(child_msg), 
        "Client closed: %d (listener: %d)", client_fd, listen_fd);
      log_res = rbuflogd_producer_log(
        &producer, 
        RBUF_LOG_LEVEL_DEBUG, 
        "child", 
        child_msg);

      if(log_res < 0) {
        printf("Logging error %d \n", log_res);
      }
      else {
        printf("Logged successfully \n");
      }

      return 0;
    }
    else {
      close(client_fd);
      continue;
    }
  }

  return 0;
}
