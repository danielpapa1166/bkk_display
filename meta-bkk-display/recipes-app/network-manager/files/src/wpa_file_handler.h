#ifndef WPA_FILE_HANDLER_H
#define WPA_FILE_HANDLER_H


#include <stddef.h>
typedef enum {
  WPA_CONFIG_ACCESS_POINT,
  WPA_CONFIG_WIFI_CLIENT, 
  WPA_CONFIG_UNKNOWN
} wpa_config_type_t;

typedef enum {
  WIFI_CRED_LOAD_SUCCESS,
  WIFI_CRED_FILE_NOT_FOUND,
  WIFI_CRED_FILE_OTHER_ERROR, 
  WIFI_CRED_LOAD_ERROR, 
  WIFI_CRED_JSON_ERROR,
} wifi_cred_load_status_t;

typedef enum {
  WPA_FILE_CONFIG_SUCCESS,
  WPA_FILE_CONFIG_ERROR
} wpa_file_config_stat_t; 

// clears Wifi Protected Access (WPA) configuration files for the given config_type
wpa_file_config_stat_t clear_wpa_config(wpa_config_type_t config_type);


wifi_cred_load_status_t load_wifi_credentials(
    const char *json_path,
    char *ssid_out, size_t ssid_len,
    char *psk_out,  size_t psk_len);

// loads Wifi config JSON file 
// and writes Wifi Protected Access (WPA) configuration files for the given config_type
wpa_file_config_stat_t write_wpa_config(wpa_config_type_t config_type); 

wpa_file_config_stat_t get_wpa_config_paths(
  wpa_config_type_t config_type,
  char *wpa_cfg_path_out, size_t wpa_cfg_path_len,
  char *network_cfg_path_out, size_t network_cfg_path_len);


#endif /* WPA_FILE_HANDLER_H */