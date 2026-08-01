#include <stdio.h>
#include <string.h>

#include <network_manager_pub.h>
#include <bkk_utils/bkk_dbus_broadcast_client.h>

static int handle_network_mode_change(
    const char *signal_value, size_t signal_value_length, void *user_data)
{
  const network_manager_data_t *network_data =
    (const network_manager_data_t *)signal_value;

  (void)user_data;

  if (signal_value_length != sizeof(*network_data)) {
    fprintf(stderr, "Unexpected network mode broadcast size: %zu\n",
      signal_value_length);
    return -1;
  }

  printf("Network mode broadcast: %d\n", network_data->mode);
  return 0;
}

int main(void)
{
  bc_client_t client;
  bkk_dbus_listener_t listener;
  bc_client_request_t request;
  bc_data_un response;
  int result;

  result = init_broadcast_client(
    NETWORK_MANAGER_DBUS_NAME,
    &listener,
    handle_network_mode_change,
    &client,
    &client);
  if (result != 0) {
    fprintf(stderr, "Failed to connect to Network Manager D-Bus: %d\n", result);
    return 1;
  }

  memset(&request, 0, sizeof(request));
  request.request[0] = 'a';

  result = send_client_request(
    NETWORK_MANAGER_DBUS_NAME, &request, &response.bc_server_data);
  if (result != 0) {
    fprintf(stderr, "Failed to fetch Network Manager mode: %d\n", result);
    bkk_dbus_deinit_listener(&listener);
    return 1;
  }

  printf("Network Manager mode: %d\n", response.network_manager_data.mode);
  bkk_dbus_deinit_listener(&listener);
  return 0;
}