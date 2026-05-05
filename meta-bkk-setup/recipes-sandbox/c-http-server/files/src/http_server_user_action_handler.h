#ifndef HTTP_SERVER_USER_ACTION_HANDLER_H
#define HTTP_SERVER_USER_ACTION_HANDLER_H

typedef struct {
  char action[16];
  char from_page[64];
  char to_page[64];
  char wifi_ssid[64];
  char wifi_password[64];
  char api_key[128];
  char station_ids[256];
} api_button_request_t;


int handle_user_action(const api_button_request_t *request);


#endif
