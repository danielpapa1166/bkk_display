#include "am_boot_mode.h"
#include <unistd.h>

static const char * api_config_flag = "/etc/bkk-display-config/api-configured"; 
static const char * wifi_config_flag = "/etc/bkk-display-config/wifi-configured";

const char * boot_mode_to_string(boot_mode_t mode) {
  switch (mode) {
    case BOOT_MODE_WIFI_CONFIG:
      return "WIFI_CONFIG";
    case BOOT_MODE_API_CONFIG:
      return "API_CONFIG";
    case BOOT_MODE_NORMAL:
      return "NORMAL";
    case BOOT_MODE_UNDEFINED:
      return "UNDEFINED";
    default:
      return "UNKNOWN_BOOT_MODE";
  }
}

boot_mode_t determine_boot_mode(void) {
  const int api_configured = (
    access(api_config_flag, F_OK) == 0
  );

  const int wifi_configured = (
    access(wifi_config_flag, F_OK) == 0
  );


  if (!api_configured && !wifi_configured) {
    // PHASE 1: No config at all, 
    // start in Wifi config mode
    return BOOT_MODE_WIFI_CONFIG;
  } 
  else if (!api_configured && wifi_configured) {
    // PHASE 2: Wifi configured but API not configured, 
    // start in API config mode
    return BOOT_MODE_API_CONFIG;
  } 
  else if (api_configured && wifi_configured) {
    // PHASE 3: Both configured, 
    // start in normal operation mode
    return BOOT_MODE_NORMAL;
  }
  else {
    return BOOT_MODE_UNDEFINED;
  }
}