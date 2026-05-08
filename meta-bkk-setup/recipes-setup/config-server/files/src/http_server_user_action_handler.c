#include "http_server_user_action_handler.h"
#include "http_server_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#define ACTION_TYPE_NEXT_STR                "next"
#define ACTION_TYPE_BACK_STR                "back"

#define ACTION_PAGE_WIFI                    "wifi"
#define ACTION_PAGE_API_KEY                 "api-key"
#define ACTION_PAGE_STATION_IDS             "stations"

static const char *TAG = "usr_act";

static int usr_act_wifi_apply(const api_button_request_t *request);

int handle_user_action(const api_button_request_t *request) {

  if (strcmp(request->action, ACTION_TYPE_NEXT_STR) == 0) {
    if(strcmp(request->from_page, ACTION_PAGE_WIFI) == 0) {
      const int wifi_result = usr_act_wifi_apply(request);
    }
    else if(strcmp(request->from_page, ACTION_PAGE_API_KEY) == 0) {
      printf("Apply API key setup: %s\n", request->api_key);
    }
    else if(strcmp(request->from_page, ACTION_PAGE_STATION_IDS) == 0) {
      printf("Apply station IDs setup: %s\n", request->station_ids);
    }
    else {
      printf("Unknown source page: '%s'\n", request->from_page);
      return -1;
    }

  }
  else if (strcmp(request->action, ACTION_TYPE_BACK_STR) == 0) {
    printf("Handling 'back' action\n");
  }
  else {
    printf("Unknown action: '%s'\n", request->action);
    return -1;
  
  }

  return 0; 
}


static int usr_act_wifi_apply(const api_button_request_t *request) {
  printf("Validating WIFI credentials for SSID: %s\n", request->wifi_ssid);

  const int wifi_validation_result = 42; // Placeholder for actual validation logic

  log_info(TAG, "WIFI config applied");

  // Create config directory if it does not exist
  const char *config_dir = "/etc/bkk-display-config";
  if (mkdir(config_dir, 0755) != 0 && errno != EEXIST) {
    log_error(TAG, "Failed to create config directory");
    return -1;
  }

  FILE *flag_file = fopen("/etc/bkk-display-config/wifi-configured", "w");
  if (flag_file == NULL) {
    log_error(TAG, "Failed to create wifi-configured file");
    return -1;
  }
  fprintf(flag_file, "1");
  fclose(flag_file);

  // Schedule a reboot in x seconds
  log_info(TAG, "Config written, scheduling device reboot in 2 seconds");
  system("(sleep 2 && reboot) &");

  return wifi_validation_result;
}