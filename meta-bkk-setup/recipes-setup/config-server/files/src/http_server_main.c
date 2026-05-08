#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "http_server_client_handler.h"
#include "http_server_config.h"
#include "rbuflogd/pub_common_types.h"
#include <rbuflogd/producer.h>

#define PORT 8080

/* -------------------------------------------------------------------------
 * Argument parsing
 * ------------------------------------------------------------------------- */
static int parse_args(int argc, char *argv[], server_mode_t *out_mode)
{
    const char *mode_prefix = "--mode=";
    const size_t mode_prefix_len = 7; /* strlen("--mode=") */

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], mode_prefix, mode_prefix_len) == 0) {
            const char *value = argv[i] + mode_prefix_len;
            if (strcmp(value, "wifi") == 0) {
                *out_mode = SERVER_MODE_WIFI;
                return 0;
            }
            if (strcmp(value, "api") == 0) {
                *out_mode = SERVER_MODE_API;
                return 0;
            }
            fprintf(stderr, "Unknown --mode value: '%s' (expected 'wifi' or 'api')\n", value);
            return -1;
        }
    }

    fprintf(stderr, "Usage: %s --mode=<wifi|api>\n", argv[0]);
    return -1;
}

static struct sockaddr_in server_addr = { 0 };


int main(int argc, char *argv[])
{  

  printf("Starting logger\n");
  rbuflogd_producer_t producer;
  rbuflogd_producer_open(
    &producer, 
    "http_srv");

  printf("logger started\n");

  server_mode_t mode = SERVER_MODE_WIFI;
  if (parse_args(argc, argv, &mode) != 0) {
    rbuflogd_producer_log(
      &producer, 
      RBUF_LOG_LEVEL_ERROR, 
      "init", 
      "Failed to parse arguments");
    rbuflogd_producer_close(&producer);
    return 1;
  }



  const char *mode_str = (mode == SERVER_MODE_API) ? "api" : "wifi";
  printf("HTTP server starting in mode: %s\n", mode_str);
  rbuflogd_producer_log(&producer, RBUF_LOG_LEVEL_INFO, "init",
    (mode == SERVER_MODE_API)
        ? "HTTP server starting in API mode"
        : "HTTP server starting in WiFi mode");

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
      close(listen_fd);
      client_handler(client_fd, &producer, mode);
      return 0;
    }
    else {
      close(client_fd);
      continue;
    }
  }

  return 0;
}
