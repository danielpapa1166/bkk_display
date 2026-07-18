#include <linux/limits.h>
#include <stdio.h>
#include <rbuflogd/logger.h>
#include <stdlib.h>
#include <string.h>

#include "wpa_file_handler.h"
#include "networkd_status.h"
#include "supplicant_handler.h"

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

static int init_clearall(void);
static int switch_network_mode(wpa_config_type_t config_type); 

// ----------------------------------------------------------------------------
// local variable definitions:
// ----------------------------------------------------------------------------
wpa_config_type_t current_network_mode = WPA_CONFIG_UNKNOWN;



int main(int argc, char *argv[]) {

  wpa_config_type_t config_type;
  int res; 

  rbuflogd_logger_init("NetMngr");
  log_info("Init", "Network Manager started successfully.");

  if(argc < 2) {
    log_error("Init", "Missing argument: network_mode=<value>");
    return -1;
  }


  if(strcmp(argv[1], "network_mode=ap") == 0) {
    config_type = WPA_CONFIG_ACCESS_POINT;
  } 
  else if(strcmp(argv[1], "network_mode=wifi") == 0) {
    config_type = WPA_CONFIG_WIFI_CLIENT;
  } 
  else {
    log_error("Init", "Invalid argument: network_mode=<value>");
    return -1;
  }

  printf("Network Manager starting with mode: %s\n", argv[1]);

  printf("Killing any existing wpa_supplicant processes...\n");
  kill_all_supplicant_processes();

  printf("Clearing all existing WPA config files...\n");
  res = init_clearall();
  if(res != 0) {
    log_error("Init", "Failed to clear existing network mode.");
    return -1;
  }

  printf("Switching network mode to: %s\n", argv[1]);
  res = switch_network_mode(config_type);
  if(res != 0) {
    log_error("Init", "Failed to switch network mode.");
    return -1;
  }


  char wpa_cfg_path[PATH_MAX];
  char network_cfg_path[PATH_MAX];
  res = get_wpa_config_paths(
    config_type,
    wpa_cfg_path, sizeof(wpa_cfg_path),
    network_cfg_path, sizeof(network_cfg_path));
  if(res != WPA_FILE_CONFIG_SUCCESS) {
    log_error("Init", "Failed to get WPA config paths.");
    return -1;
  }

  printf("Starting wpa_supplicant with config: %s\n", wpa_cfg_path);
  res = start_supplicant(wpa_cfg_path, "wlan0");
  if(res != 0) {
    log_error("Init", "Failed to start wpa_supplicant.");
    return -1;
  }

  printf("Network Manager started successfully in mode: %s\n", argv[1]);
  

  return 0;
}


// ----------------------------------------------------------------------------
// local function implementations
// ----------------------------------------------------------------------------

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

  const int networkd_status = networkd_check_status(30);
  if (networkd_status != 0) {
    log_error("Sw Net", "systemd-networkd is not active.");
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

  const wpa_file_config_stat_t write_status = write_wpa_config(config_type);
  if (write_status != WPA_FILE_CONFIG_SUCCESS) {
    log_error("Sw Net", "Failed to write WPA config files.");
    return -1;
  }

  const int reload_status = reload_networkd_config();
  if (reload_status != 0) {
    log_error("Sw Net", "Failed to reload systemd-networkd config.");
    return -1;
  }

  current_network_mode = config_type;
  log_info("Sw Net", "Network mode switched successfully.");
  return 0;
}

