#include <stdio.h>
#include <string.h>
#include "wpa_config.h"


typedef enum {
  BOOT_MODE_WIFI_CONFIG,
  BOOT_MODE_API_CONFIG,
  BOOT_MODE_NORMAL,
  BOOT_MODE_UNKNOWN
} boot_mode_t;

static boot_mode_t parse_args(int argc, char *argv[]);


int main(int argc, char *argv[])
{
  const boot_mode_t boot_mode = parse_args(argc, argv);

  if(boot_mode == BOOT_MODE_UNKNOWN) {
    // todo log here
    return -1;
  }

  const char * wpa_cfg_str = NULL;
  const char * network_cfg_str = NULL;
  
  if(boot_mode == BOOT_MODE_WIFI_CONFIG) {
    wpa_cfg_str = WPA_CONFIG_AP;
    network_cfg_str = NETWORK_CONFIG_AP;
  }

  return 0;
}


static boot_mode_t parse_args(int argc, char *argv[]) {
  boot_mode_t boot_mode = BOOT_MODE_UNKNOWN;

  if (argc < 2) {
    return boot_mode;
  }

  if(strcmp(argv[1], "boot_mode=wifi_config") == 0) {
    boot_mode = BOOT_MODE_WIFI_CONFIG;
  }
  else if(strcmp(argv[1], "boot_mode=api_config") == 0) {
    boot_mode = BOOT_MODE_API_CONFIG;
  }
  else if (strcmp(argv[1], "boot_mode=normal") == 0) {
    boot_mode = BOOT_MODE_NORMAL;
  }

  return boot_mode;
}