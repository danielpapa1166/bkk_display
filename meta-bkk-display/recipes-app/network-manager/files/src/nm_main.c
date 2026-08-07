#include <linux/limits.h>
#include <stdio.h>
#include <rbuflogd/logger.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wpa_file_handler.h"
#include "networkd_status.h"
#include "supplicant_handler.h"
#include "wpa_file_config.h"
#include <bkk_utils/bkk_dbus_broadcast_server.h>
#include <bkk_utils/bkk_dbus.h>
#include <network_manager_pub.h>
#include <stdbool.h>

#define DBUS_PEER_NAME "bkk-network-manager"

// IP: networkctl status wlan0


/* 
valgrind debug: 

IMAGE_INSTALL:append = " network-manager-dbg"

ls -l /usr/lib/debug/usr/bin/network_manager.debug
readelf -S /usr/bin/network_manager | grep debug

valgrind \
  --tool=memcheck \
  --num-callers=40 \
  --track-origins=yes \
  --read-var-info=yes \
  --error-limit=no \
  --leak-check=full \
  --show-leak-kinds=all \
  --log-file=/tmp/network_manager.valgrind \
  /usr/bin/network_manager network_mode=wifi

cat /tmp/network_manager.valgrind
*/

// ----------------------------------------------------------------------------
// local function definitions:
// ----------------------------------------------------------------------------


static wpa_config_type_t load_network_mode(void);
static int init_clearall(void);
static int switch_network_mode(wpa_config_type_t config_type); 
static void broadcast_network_mode_change(wpa_config_type_t new_mode);
static int dbus_client_handler(
  const char *sigvalue, size_t sigvalue_len, void *user_data);

// ----------------------------------------------------------------------------
// local variable definitions:
// ----------------------------------------------------------------------------
static wpa_config_type_t current_network_mode = WPA_CONFIG_UNKNOWN;
static bc_server_t bc_server;  // Broadcast server instance
static bkk_dbus_listener_t bc_listener;  // Broadcast listener instance
static char msg[256];  // Message buffer for logging
static bool is_bc_server_up = false;  // the broadcast server is up


int main(int argc, char *argv[]) {

  wpa_config_type_t config_type;
  int res; 

  rbuflogd_logger_init("NetMngr");
  log_info("Init", "Network Manager started successfully. Wait for DBUS ... ");

  (void) wait_for_dbus_connection(-1);
  log_debug("Init", "D-Bus connection established, proceeding with initialization.");
  
  // Initialize broadcast server
  int bc_init_res; 
  int retry_counter = 0;
  do {
    bc_init_res = init_broadcast_server(
      NETWORK_MANAGER_DBUS_NAME,
      DBUS_PEER_NAME,
      &bc_listener,
      dbus_client_handler,
      &bc_server,
      &bc_server
    );
    sleep(1);
    retry_counter++;
    if(retry_counter > 5) {
      log_error("Init", "Exceeded max retries for initializing broadcast server");
      return 1;
    }
  } while(bc_init_res != 0);

  if (bc_init_res != 0) {
    log_warning("Init", "Failed to initialize D-Bus broadcast server");
    // no exit, continue to run, but without broadcast functionality
  }
  else {
    is_bc_server_up = true;
    log_info("Init", "D-Bus broadcast server initialized successfully");
  }

  config_type = load_network_mode();

  snprintf(msg, sizeof(msg), "Initial network mode: %s", 
    (config_type == WPA_CONFIG_ACCESS_POINT) ? "AP" : "WiFi Client");
  log_info("Init", msg);
  printf("%s\n", msg);

  res = switch_network_mode(config_type);
  if(res != 0) {
    log_error("Init", "Failed to switch network mode.");
    return -1;
  }

  snprintf(msg, sizeof(msg), "Network Manager started %s in mode: %s", 
    (bc_init_res == 0) ? "successfully" : "with warnings (see logs)",
    (config_type == WPA_CONFIG_ACCESS_POINT) ? "AP" : "WiFi Client");
  log_info("Init", msg);

  printf("%s\n", msg);



  while(1) {
    const wpa_config_type_t new_config_type = load_network_mode();
    if(new_config_type != current_network_mode) {
      printf("Detected network mode change. Switching to: %s\n",
        (new_config_type == WPA_CONFIG_ACCESS_POINT) ? "AP" : "WiFi Client");
      res = switch_network_mode(new_config_type);
      if(res != 0) {
        log_error("Sw Net", "Failed to switch network mode.");
      }
      current_network_mode = new_config_type;

      // Broadcast the network mode change to clients
      broadcast_network_mode_change(new_config_type);
    }
    sleep(2);
  }
  

  return 0;
}


// ----------------------------------------------------------------------------
// local function implementations
// ----------------------------------------------------------------------------

static wpa_config_type_t load_network_mode(void) {

  char ssid[SSID_MAX_LEN];
  char psk[PSK_MAX_LEN];


  const wifi_cred_load_status_t load_wifi_cred_stat = load_wifi_credentials(
    WPA_WIFI_CREDENTIALS_JSON,
    ssid, sizeof(ssid),
    psk,  sizeof(psk));

  if (load_wifi_cred_stat == WIFI_CRED_LOAD_SUCCESS) {
    return WPA_CONFIG_WIFI_CLIENT;
  }
  else {
    return WPA_CONFIG_ACCESS_POINT;
  }
}


static int init_clearall(void) {
  log_info("Init", "Clearing all existing WPA config files.");
  wpa_file_config_stat_t clear_ap_status = clear_wpa_config(
    WPA_CONFIG_ACCESS_POINT);
  if (clear_ap_status != WPA_FILE_CONFIG_SUCCESS) {
    log_error("Init", "Failed to clear AP WPA config files.");
    return -1;
  }

  wpa_file_config_stat_t clear_wifi_status = clear_wpa_config(
    WPA_CONFIG_WIFI_CLIENT);
  if (clear_wifi_status != WPA_FILE_CONFIG_SUCCESS) {
    log_error("Init", "Failed to clear WiFi client WPA config files.");
    return -1;
  }

  log_info("Init", "All existing WPA config files cleared successfully.");
  return 0;
}


static int switch_network_mode(wpa_config_type_t config_type) {
  
  if(current_network_mode == config_type) {
    log_info("Sw Net", "Network mode already set to requested mode.");
    return 0;
  }

  // Check if systemd-networkd is active before proceeding
  const int networkd_status = networkd_check_status(30);
  if (networkd_status != 0) {
    log_error("Sw Net", "systemd-networkd is not active.");
    return -1;
  }

  printf("Clearing all existing WPA config files...\n");
  int res = init_clearall();
  if(res != 0) {
    log_error("Init", "Failed to clear existing network mode.");
    return -1;
  }

  // clear existing config files for the given config_type
  if(current_network_mode != WPA_CONFIG_UNKNOWN) {
    const wpa_file_config_stat_t clear_status 
      = clear_wpa_config(current_network_mode);

    if (clear_status != WPA_FILE_CONFIG_SUCCESS) {
      log_error("Sw Net", "Failed to clear existing WPA config files.");
      return -1;
    }
  }

  // write new config files for the given config_type
  const wpa_file_config_stat_t write_status = write_wpa_config(config_type);
  if (write_status != WPA_FILE_CONFIG_SUCCESS) {
    log_error("Sw Net", "Failed to write WPA config files.");
    return -1;
  }

  // reload systemd-networkd to apply the new configuration
  const int reload_status = reload_networkd_config();
  if (reload_status != 0) {
    log_error("Sw Net", "Failed to reload systemd-networkd config.");
    return -1;
  }

  current_network_mode = config_type;
  log_info("Sw Net", "Network mode switched successfully.");


  // Start wpa_supplicant with the new configuration
  char wpa_cfg_path[PATH_MAX];
  char network_cfg_path[PATH_MAX];
  int get_res = get_wpa_config_paths(
    config_type,
    wpa_cfg_path, sizeof(wpa_cfg_path),
    network_cfg_path, sizeof(network_cfg_path));
  if(get_res != WPA_FILE_CONFIG_SUCCESS) {
    log_error("Init", "Failed to get WPA config paths.");
    return -1;
  }

  printf("Stopping any existing wpa_supplicant processes...\n");
  kill_all_supplicant_processes(); 

  // Start wpa_supplicant with the new configuration
  printf("Starting wpa_supplicant with config: %s\n", wpa_cfg_path);
  int start_res = start_supplicant(wpa_cfg_path, "wlan0");
  if(start_res != 0) {
    log_error("Init", "Failed to start wpa_supplicant.");
    return -1;
  }


  return 0;
}


static void broadcast_network_mode_change(wpa_config_type_t new_mode) {

  if(!is_bc_server_up) {
    log_warning("Broadcast", "Broadcast server is not up. "
      "Cannot broadcast network mode change.");
    return;
  }

  bc_data_un data;
  data.network_manager_data.mode = (new_mode == WPA_CONFIG_ACCESS_POINT) 
    ? NETWORK_MANAGER_MODE_ACCESS_POINT 
    : NETWORK_MANAGER_MODE_WIFI_CLIENT;


  serve_data(
    &bc_server,
    &data.bc_server_data
  );

  printf("Broadcasted network mode change: %s\n", 
    (new_mode == WPA_CONFIG_ACCESS_POINT) ? "AP" : "WiFi Client");
  
  log_debug("Broadcast", "Network mode change broadcasted successfully.");

}


static int dbus_client_handler(
    const char *sigvalue, size_t sigvalue_len, void *user_data) {
  
  if(!is_bc_server_up) {
    log_warning("DBus Handler", "Broadcast server is not up. "
      "Cannot handle client requests.");
    return -1;
  }

  (void) user_data; 

  bc_client_request_t* received_data = (bc_client_request_t*)sigvalue;
  bc_server_t *server = (bc_server_t*)user_data;


  // do something with the data: 
  printf("NM REQUEST HANDLER: Received request: %s\n", received_data->request);

  bc_data_un data;
  data.network_manager_data.mode = (current_network_mode == WPA_CONFIG_ACCESS_POINT) 
    ? NETWORK_MANAGER_MODE_ACCESS_POINT 
    : NETWORK_MANAGER_MODE_WIFI_CLIENT;


  serve_data(
    server,
    &data.bc_server_data
  );

  printf("Response send. \n"); 
  log_debug("DBus Handler", "Response sent to client successfully.");
  return 0;
}
