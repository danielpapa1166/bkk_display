#ifndef WPA_CONFIG_H
#define WPA_CONFIG_H

#include <string.h>

// Default paths and filenames 
#define WPA_CFG_PATH_DEFAULT         "/run"
#define WPA_CFG_NAME_DEFAULT         "wpa_supplicant-ap.conf"
#define NETWORK_CFG_PATH_DEFAULT     "/run/systemd/network"
#define NETWORK_CFG_NAME_DEFAULT     "20-wlan0-ap.network"

typedef struct {
  const char * wpa_cfg_path;
  const char * network_cfg_path;
  const char * wpa_cfg_name;
  const char * network_cfg_name;
  const char * wpa_cfg_str;
  const char * network_cfg_str;
} wpa_config_t;


static const char WPA_CONFIG_AP[] =
    "ctrl_interface=/run/wpa_supplicant\n"
    "update_config=1\n"
    "country=HU\n"
    "\n"
    "network={\n"
    "    ssid=\"BKK-Display-Setup\"\n"
    "    mode=2\n"
    "    key_mgmt=NONE\n"
    "    frequency=2437\n"
    "}";

static const char NETWORK_CONFIG_AP[] =
    "[Match]\n"
    "Name=wlan0\n"
    "\n"
    "[Network]\n"
    "Address=192.168.4.1/24\n"
    "DHCPServer=yes\n"
    "\n"
    "[DHCPServer]\n"
    "PoolOffset=10\n"
    "PoolSize=50";

/* NOTE: network_cfg_path default requires /run/systemd to already exist.
 * This is guaranteed at boot by systemd, but prepare_config_folder only
 * performs a single-level mkdir. */
static wpa_config_t wpa_ap_config = {
  .wpa_cfg_path         = WPA_CFG_PATH_DEFAULT,
  .network_cfg_path     = NETWORK_CFG_PATH_DEFAULT,
  .wpa_cfg_name         = WPA_CFG_NAME_DEFAULT,
  .network_cfg_name     = NETWORK_CFG_NAME_DEFAULT,
  .wpa_cfg_str          = WPA_CONFIG_AP,
  .network_cfg_str      = NETWORK_CONFIG_AP
};

static wpa_config_t wpa_wifi_config; 

#endif /* WPA_CONFIG_H */