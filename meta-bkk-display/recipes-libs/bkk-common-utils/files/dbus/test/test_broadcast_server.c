#include "test_protocol_def.h"
#include "bkk_dbus_broadcast_server.h"
#include <stdio.h>
#include <string.h>


// defined by the user: 
static int client_request_handler(const char* sigvalue, size_t sigvalue_len, void* user_data) {
  printf("Signal received: %s (length: %zu)\n", sigvalue, sigvalue_len);

  
  bc_client_request_t* received_data = (bc_client_request_t*)sigvalue;
  bc_server_t *server = (bc_server_t*)user_data;

  // do something with the data: 
  printf("Received request: %s\n", received_data->request);

  // send a response back to the client:
  bc_server_data_t server_data;
  snprintf(server_data.server_data, sizeof(server_data.server_data), 
    "Response from server: %s", received_data->request);

  serve_data(
    server,
    &server_data
  );

  return 0;
}



int main() {

  bc_server_t server;
  bkk_dbus_listener_t clt;
  const char * bus_name = BC_TEST_BUS;
  init_broadcast_server(
    BC_TEST_BUS,
    &clt,
    client_request_handler,
    &server,
    &server
  );
  printf("Hello, World!\n");

  while(1) {
    sleep(1);
    bc_server_data_t server_data;
    snprintf(server_data.server_data, sizeof(server_data.server_data), 
      "Response from server: %s", "Periodic update");

    serve_data(
      &server,
      &server_data
    );
  }

  pthread_join(clt.thread, NULL);


  return 0; 
}