#include "test_protocol_def.h"
#include "bkk_dbus_broadcast_client.h"
#include <stdio.h>
#include <string.h>


#define BC_TEST_PEER_NAME "bkk-dbus-test-client"


static int sig_handler(const char* sigvalue, size_t sigvalue_len, void* user_data) {
  printf("Signal received: %s (length: %zu)\n", sigvalue, sigvalue_len);
  return 0;
}


int main(int argc, char *argv[]) {
  bc_client_t client;
  bkk_dbus_listener_t clt;
  const char * bus_name = BC_TEST_BUS;
  init_broadcast_client(
    BC_TEST_BUS,
    BC_TEST_PEER_NAME,
    &clt,
    sig_handler,
    &client,
    &client
  );


  int client_type = 0; 

  if(argc > 1) {
    if(strcmp(argv[1], "send_once") == 0) {
      client_type = 1;
    } 
    else if(strcmp(argv[1], "wait_forever") == 0) {
      client_type = 2;
    }
    else if(strcmp(argv[1], "send_and_wait") == 0) {
      client_type = 3;
    }
    else {
      fprintf(stderr, "Unknown client type: %s\n", argv[1]);
      return 1;
    }
  }

  if(client_type == 1) {
    bc_client_request_t request;
    snprintf(request.request, sizeof(request.request), "Hello from client!");
    send_client_request(&client, &request, NULL);
    bkk_dbus_deinit_listener(&clt);
  }
  else if(client_type == 2) {
    pthread_join(clt.thread, NULL);
  }
  else if(client_type == 3) {
    bc_client_request_t request;
    snprintf(request.request, sizeof(request.request), "Hello from client!");
    bc_server_data_t response;
    send_client_request(&client, &request, &response);
    printf("Received response: %s\n", response.server_data);
    pthread_join(clt.thread, NULL);
    bkk_dbus_deinit_listener(&clt);
  }


  return 0;
}