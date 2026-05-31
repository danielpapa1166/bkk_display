#include "http_server_user_action_handler.h"
#include "rbuflogd/logger.h"
#include "cJSON.h"
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
static int usr_act_api_key_apply(const api_button_request_t *request);
static int usr_act_station_ids_apply(const api_button_request_t *request);

int handle_user_action(
    const api_button_request_t *request, server_mode_t mode) {
  (void) mode;

  char msg[100];
  snprintf(msg, sizeof(msg), 
    "Handling user action: %s from page: %s", request->action, request->from_page);
  log_debug(TAG, msg);

  if (strcmp(request->action, ACTION_TYPE_NEXT_STR) == 0) {
    if(strcmp(request->from_page, ACTION_PAGE_WIFI) == 0) {
      const int wifi_result = usr_act_wifi_apply(request);
    }
    else if(strcmp(request->from_page, ACTION_PAGE_API_KEY) == 0) {
      const int api_key_result = usr_act_api_key_apply(request);
    }
    else if(strcmp(request->from_page, ACTION_PAGE_STATION_IDS) == 0) {
      const int station_ids_result = usr_act_station_ids_apply(request);
    }
    else {
      char msg[100]; 
      snprintf(msg, sizeof(msg), 
      "Unknown source page: %s", request->from_page);
      log_error(TAG, msg);
      return -1;
    }

  }
  else if (strcmp(request->action, ACTION_TYPE_BACK_STR) == 0) {
    // handling back action if needed, currently no specific logic for back action
  }
  else {
    char msg[100]; 
    snprintf(msg, sizeof(msg), 
    "Unknown action: %s", request->action);
    log_error(TAG, msg);
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

  // Persist WiFi credentials so wpa_helper can read them in API_CONFIG mode
  cJSON *wifi_json = cJSON_CreateObject();
  cJSON_AddStringToObject(wifi_json, "ssid",     request->wifi_ssid);
  cJSON_AddStringToObject(wifi_json, "password", request->wifi_password);
  char *json_str = cJSON_PrintUnformatted(wifi_json);
  cJSON_Delete(wifi_json);
  if (json_str == NULL) {
    log_error(TAG, "Failed to serialise wifi credentials to JSON");
    return -1;
  }
  FILE *cred_file = fopen("/etc/bkk-display-config/wifi_config.json", "w");
  if (cred_file == NULL) {
    log_error(TAG, "Failed to open wifi_config.json for writing");
    free(json_str);
    return -1;
  }
  fputs(json_str, cred_file);
  fclose(cred_file);
  free(json_str);
  log_info(TAG, "WiFi credentials saved to wifi_config.json");

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


static int usr_act_api_key_apply(const api_button_request_t *request) {
  printf("Applying API key: %s\n", request->api_key);

  const int api_key_validation_result = 42; // Placeholder for actual validation logic

  log_info(TAG, "API key applied");

  return api_key_validation_result;
}

static int usr_act_station_ids_apply(const api_button_request_t *request) {
  printf("Applying station IDs: %s\n", request->station_ids);

  // Create /etc/bkk-api directory if it does not exist
  const char *bkk_api_dir = "/etc/bkk-api";
  if (mkdir(bkk_api_dir, 0755) != 0 && errno != EEXIST) {
    log_error(TAG, "Failed to create /etc/bkk-api directory");
    return -1;
  }

  // --- Write api-key.txt ---
  if (request->api_key[0] != '!') {
    FILE *key_file = fopen("/etc/bkk-api/api-key.txt", "w");
    if (key_file == NULL) {
      log_error(TAG, "Failed to open /etc/bkk-api/api-key.txt for writing");
      return -1;
    }
      
    fprintf(key_file, "%s", request->api_key);
    fclose(key_file);
    log_info(TAG, "API key written to /etc/bkk-api/api-key.txt");
  }
  else {
    log_info(TAG, "API key starts with '!', skipping api-key.txt write");
  }

  // --- Build stations JSON and write config.json ---
  cJSON *root = cJSON_CreateObject();
  if (!root) {
    log_error(TAG, "Failed to create JSON object");
    return -1;
  }

  cJSON *stations_array = cJSON_AddArrayToObject(root, "stations");
  if (!stations_array) {
    log_error(TAG, "Failed to create stations JSON array");
    cJSON_Delete(root);
    return -1;
  }

  // Tokenise station_ids by whitespace and commas
  char ids_copy[sizeof(request->station_ids)];
  strncpy(ids_copy, request->station_ids, sizeof(ids_copy) - 1);
  ids_copy[sizeof(ids_copy) - 1] = '\0';

  char *token = strtok(ids_copy, " ,\t\n\r");
  while (token != NULL) {
    if (token[0] != '\0') {
      cJSON_AddItemToArray(stations_array, cJSON_CreateString(token));
    }
    token = strtok(NULL, " ,\t\n\r");
  }

  char *json_str = cJSON_Print(root);
  cJSON_Delete(root);
  if (!json_str) {
    log_error(TAG, "Failed to serialise config JSON");
    return -1;
  }

  FILE *config_file = fopen("/etc/bkk-api/config.json", "w");
  if (config_file == NULL) {
    log_error(TAG, "Failed to open /etc/bkk-api/config.json for writing");
    free(json_str);
    return -1;
  }
  fprintf(config_file, "%s\n", json_str);
  fclose(config_file);
  free(json_str);
  log_info(TAG, "Station IDs written to /etc/bkk-api/config.json");

  // --- Write api-configured flag ---
  const char *config_dir = "/etc/bkk-display-config";
  if (mkdir(config_dir, 0755) != 0 && errno != EEXIST) {
    log_error(TAG, "Failed to create /etc/bkk-display-config directory");
    return -1;
  }

  FILE *flag_file = fopen("/etc/bkk-display-config/api-configured", "w");
  if (flag_file == NULL) {
    log_error(TAG, "Failed to open api-configured file for writing");
    return -1;
  }
  fprintf(flag_file, "1");
  fclose(flag_file);

  log_info(TAG, "Config written, scheduling device reboot in 2 seconds");
  system("(sleep 2 && reboot) &");

  return 0;
}