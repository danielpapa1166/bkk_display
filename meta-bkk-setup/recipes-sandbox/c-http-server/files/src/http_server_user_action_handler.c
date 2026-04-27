#include "http_server_user_action_handler.h"
#include <stdio.h>
#include <string.h>

#define ACTION_TYPE_NEXT_STR                "next"
#define ACTION_TYPE_BACK_STR                "back"

#define ACTION_PAGE_WIFI                    "wifi"
#define ACTION_PAGE_API_KEY                 "api-key"
#define ACTION_PAGE_STATION_IDS             "stations"



int handle_user_action(const api_button_request_t *request) {

  if (strcmp(request->action, ACTION_TYPE_NEXT_STR) == 0) {
    if(strcmp(request->from_page, ACTION_PAGE_WIFI) == 0) {
      printf("Apply WIFI setup: SSID: %s, PW: %s\n", 
        request->wifi_ssid, request->wifi_password);
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