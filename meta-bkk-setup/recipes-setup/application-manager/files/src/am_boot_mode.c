#include "am_boot_mode.h"
#include "am_types.h"
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#define AM_DEFAULT_FLAGS_DIR "/etc/bkk-display-config"
static const char * api_flag_name  = "api-configured";
static const char * wifi_flag_name = "wifi-configured";


static boot_mode_t boot_mode = BOOT_MODE_UNDEFINED; 


boot_mode_t get_boot_mode() {
  return boot_mode;
}

boot_mode_t determine_boot_mode(const char * flags_dir) {
  if (flags_dir == NULL) {
    flags_dir = AM_DEFAULT_FLAGS_DIR;
  }

  char api_path[PATH_MAX];
  char wifi_path[PATH_MAX];
  snprintf(api_path,  sizeof(api_path),  "%s/%s", flags_dir, api_flag_name);
  snprintf(wifi_path, sizeof(wifi_path), "%s/%s", flags_dir, wifi_flag_name);

  const int api_configured = (
    access(api_path, F_OK) == 0
  );

  const int wifi_configured = (
    access(wifi_path, F_OK) == 0
  );


  if (!api_configured && !wifi_configured) {
    // PHASE 1: No config at all, 
    // start in Wifi config mode
    boot_mode = BOOT_MODE_WIFI_CONFIG;
  } 
  else if (!api_configured && wifi_configured) {
    // PHASE 2: Wifi configured but API not configured, 
    // start in API config mode
    boot_mode = BOOT_MODE_API_CONFIG;
  } 
  else if (api_configured && wifi_configured) {
    // PHASE 3: Both configured, 
    // start in normal operation mode
    boot_mode = BOOT_MODE_NORMAL;
  }
  else {
    boot_mode = BOOT_MODE_UNDEFINED;
  }

  return boot_mode; 
}