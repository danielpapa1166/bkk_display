#include <stdio.h>
#include <string.h>
#include <chttp.h>
#include "http_server_get_handler.h"
#include "http_server_post_handler.h"
#include "rbuflogd/logger.h"
#include "http_server_user_action_handler.h"
#include "config_server_pub.h"
#include <network_manager_pub.h>
#include <bkk_utils/bkk_dbus_broadcast_client.h>
#include <bkk_utils/bkk_dbus_broadcast_server.h>
#include <bkk_utils/bkk_dbus.h>
#include <pthread.h>
#include <unistd.h>

#define PORT 8080
#define DBUS_PEER_NAME "bkk-http-config"


static int setup_broadcast_client(bc_client_t *client, bkk_dbus_listener_t *listener);
static int setup_broadcast_server(bc_server_t *server, bkk_dbus_listener_t *listener);

static int broadcast_signal_handler(
  const char *sigvalue, size_t sigvalue_len, void *user_data);
static int broadcast_request_handler(
  const char *sigvalue, size_t sigvalue_len, void *user_data);



network_manager_mode_t online_status = NETWORK_MANAGER_MODE_UNKNOWN;

int main(int argc, char *argv[])
{
  rbuflogd_logger_init("http_srv");

  log_debug("Init", "Initializing content state machine, wait for DBUS ...");
  // check connection status: 
  (void) wait_for_dbus_connection(-1);
  log_debug("Init", "D-Bus connection established, proceeding with initialization.");



  bc_client_t client;
  bc_server_t server;
  bkk_dbus_listener_t listener_client, listener_server;
  int res;
  res = setup_broadcast_client(&client, &listener_client);
  if(res != 0) {
    log_error("Main", "Failed to setup broadcast client");
    return 1;
  }

  res = setup_broadcast_server(&server, &listener_server);
  if(res != 0) {
    log_error("Main", "Failed to setup broadcast server");
    return 1;
  }

  set_broadcast_server(&server);


  log_info("Main", "Broadcast client initialized, listening for network mode changes");


  bc_client_request_t request;
  request.request[0] = 'a';
  request.request[1] = '\0'; 
  bc_data_un response;
  const int request_res = send_client_request(
    &client,
    &request,
    &response.bc_server_data);
  if (request_res != 0) {
    log_error("Main", "Failed to request the initial network mode");
    return 1;
  }

  online_status = response.network_manager_data.mode;
  printf("Online status: %s\n", 
    (online_status == NETWORK_MANAGER_MODE_ACCESS_POINT) ? "AP" : 
    (online_status == NETWORK_MANAGER_MODE_WIFI_CLIENT) ? "WiFi Client" : 
    "Unknown");


  printf("HTTP server starting in mode: %d\n", (int) online_status);
  log_info("init",
    (online_status == NETWORK_MANAGER_MODE_WIFI_CLIENT)
        ? "HTTP server starting in WiFi mode"
        : "HTTP server starting in API mode");

  chttp_server_t *srv = chttp_server_create(PORT);
  if (srv == NULL) {
    log_error("main", "Failed to create HTTP server.");
    rbuflogd_logger_close();
    return 1;
  }

  chttp_server_register_route(srv, "GET",  "/api/mode",   http_server_handle_get_api,          &online_status);
  chttp_server_register_route(srv, "GET",  "/",           http_server_handle_resource_request, NULL);
  chttp_server_register_route(srv, "GET",  "/index.html", http_server_handle_resource_request, NULL);
  chttp_server_register_route(srv, "GET",  "/styles.css", http_server_handle_resource_request, NULL);
  chttp_server_register_route(srv, "GET",  "/app.js",     http_server_handle_resource_request, NULL);
  chttp_server_register_route(srv, "POST", "/api/button", http_server_handle_button_post,      &online_status);
  chttp_server_register_route(srv, "POST", "/api/finish", http_server_handle_finish_post,      &online_status);

  log_info("main", "HTTP server running.");
  chttp_server_run(srv);

  chttp_server_destroy(srv);
  rbuflogd_logger_close();
  return 0;
}



static int broadcast_signal_handler(
    const char *sigvalue, size_t sigvalue_len, void *user_data) {

  log_debug("br_sig", "Handling broadcast signal"); 
  (void)user_data;

  network_manager_data_t *received_data = (network_manager_data_t*)sigvalue;
  /*if (sigvalue_len != sizeof(network_manager_data_t)) {
    log_error("Broadcast", "Received broadcast signal with unexpected size");
    return -1;
  }*/

  printf("Received broadcast signal: Network mode changed to: %s\n", 
    (received_data->mode == NETWORK_MANAGER_MODE_ACCESS_POINT) ? "AP" : 
    (received_data->mode == NETWORK_MANAGER_MODE_WIFI_CLIENT) ? "WiFi Client" : 
    "Unknown");

  if (received_data->mode == NETWORK_MANAGER_MODE_ACCESS_POINT) {
    online_status = NETWORK_MANAGER_MODE_ACCESS_POINT;
    log_info("Broadcast", "Received broadcast signal: Network mode changed to Access Point");
  }
  else if (received_data->mode == NETWORK_MANAGER_MODE_WIFI_CLIENT) {
    online_status = NETWORK_MANAGER_MODE_WIFI_CLIENT;
    log_info("Broadcast", "Received broadcast signal: Network mode changed to WiFi Client");
  }
  else {
    log_info("Broadcast", "Received broadcast signal: Network mode changed to Unknown");
  }

  return 0;
}

static int broadcast_request_handler(
    const char *sigvalue, size_t sigvalue_len, void *user_data) {

  // to be implemented later, 
  // now requests are ignored, 
  // as only new data is signaled in broadcast
  log_debug("br_req", "Handling broadcast request, sending data to peers");

  bc_server_t *server = (bc_server_t*)user_data;
  bc_config_server_un data;
  data.config_server_data.signal = CONFIG_SERVER_DONT_CARE; 
  serve_data(
    server,
    &data.bc_server_data
  );

  log_debug("br_req", "Broadcast request handled successfully"); 
  return 0;
}



static int setup_broadcast_client(bc_client_t *client, bkk_dbus_listener_t *listener) {
  const char * bus_name = NETWORK_MANAGER_DBUS_NAME;
  const char * client_name = DBUS_PEER_NAME;
  int bc_res; 
  int retry_counter = 0; 
  do {
    bc_res = init_broadcast_client(
      bus_name,
      client_name,
      listener,
      broadcast_signal_handler,
      client,
      client
    );
    log_warning("Main", "broadcast client init failed, retry... "); 
    
    retry_counter ++; 
    if(retry_counter > 5) {
      log_error("Main", "Failed to initialize broadcast client");
      return 1; 
    }
    sleep(1); 
  } while(bc_res != 0); 

  if(bc_res != 0) {
    printf("Failed to initialize broadcast client, error code: %d\n", bc_res);
    log_error("Main", "Failed to initialize broadcast client");
    return 1;
  }
  log_info("Main", "Broadcast client initialized, listening for network mode changes");
  return 0; 
}

static int setup_broadcast_server(bc_server_t *server, bkk_dbus_listener_t *listener) {
  const char * bus_name = CONFIG_SERVER_DBUS_NAME;
  const char * server_name = DBUS_PEER_NAME;
  int bc_res; 
  int retry_counter = 0; 
  do {
    bc_res = init_broadcast_server(
      bus_name,
      server_name,
      listener,
      broadcast_request_handler,
      server,
      server
    );
    log_warning("Main", "broadcast server init failed, retry... "); 
    
    retry_counter ++; 
    if(retry_counter > 5) {
      log_error("Main", "Failed to initialize broadcast server");
      return 1; 
    }
    sleep(1); 
  } while(bc_res != 0); 

  if(bc_res != 0) {
    printf("Failed to initialize broadcast server, error code: %d\n", bc_res);
    log_error("Main", "Failed to initialize broadcast server");
    return 1;
  }
  log_info("Main", "Broadcast server initialized, listening for network mode changes");
  return 0; 
}