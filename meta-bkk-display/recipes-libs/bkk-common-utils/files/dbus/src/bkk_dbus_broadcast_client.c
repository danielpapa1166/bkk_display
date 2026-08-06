#include "bkk_dbus_broadcast_client.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define RESPONSE_TIMEOUT_SECONDS 1
#define REQUEST_MAX_ATTEMPTS 3

static int server_response_handler(const char* sigvalue, size_t sigvalue_len, void* user_data) {
  if (sigvalue_len < sizeof(broadcast_message_t)) {
    fprintf(stderr, "Received signal is too small to contain a valid broadcast_message_t\n");
    return -1;
  }

  const broadcast_message_t* msg = (const broadcast_message_t*)sigvalue;
  bc_client_t *client = (bc_client_t*)user_data;

  if(msg->msg_type == BC_MSG_TYPE_SERVER_DATA) {
    if (memchr(msg->peer_id, '\0', sizeof(msg->peer_id)) == NULL) {
      fprintf(stderr, "Received server data with an invalid peer ID\n");
      return -1;
    }

    const int is_broadcast = (msg->peer_id[0] == '\0');
    const int is_response_for_client =
      client->client_name != NULL && strcmp(msg->peer_id, client->client_name) == 0;

    if (!is_broadcast && !is_response_for_client) {
      return 0;
    }

    pthread_mutex_lock(&client->response_mutex);
    if (is_response_for_client && client->wait_for_resp) {
      memcpy(&client->last_response, &msg->data.server_data, sizeof(bc_server_data_t));
      client->response_available = 1;
      client->wait_for_resp = 0;
      pthread_cond_signal(&client->response_received);
      pthread_mutex_unlock(&client->response_mutex);
    }
    else {
      pthread_mutex_unlock(&client->response_mutex);
      if (is_broadcast && client->server_response_handler) {
        client->server_response_handler(
          msg->data.server_data.server_data,
          sizeof(msg->data.server_data.server_data),
          client->user_data);
      }
    }
  }
  else {
    printf("DEBUG: Received message of type %d, ignoring\n", msg->msg_type);
    // dont care 
  }

  return 0;
}


int init_broadcast_client(const char * bus_name, const char * client_name,
    bkk_dbus_listener_t* clt, bkk_dbus_listener_sig_hdl_t handler, 
    void* user_data, bc_client_t *client) {
  if (bus_name == NULL || client_name == NULL || clt == NULL || client == NULL ||
      strlen(client_name) >= BROADCAST_PEER_ID_SIZE) {
    return -1;
  }

  memset(clt, 0, sizeof(*clt));
  memset(client, 0, sizeof(*client));

  client->server_response_handler = handler;
  client->user_data = user_data;
  client->bus_name = bus_name;
  client->client_name = client_name;
  pthread_mutex_init(&client->response_mutex, NULL);
  pthread_cond_init(&client->response_received, NULL);
  char BROADCAST_BUS_NAME[256];
  snprintf(
    BROADCAST_BUS_NAME,
    sizeof(BROADCAST_BUS_NAME),
    BROADCAST_BUS_NAME_TEMPLATE,
    bus_name);

  const bkk_dbus_err_t init_stat = bkk_dbus_init_listener(
    BROADCAST_BUS_NAME,
    clt,
    &server_response_handler,
    client
  );

  return (init_stat == bkk_dbus_err_none) ? 0 : -1;
}


int send_client_request(bc_client_t *client,
  const bc_client_request_t *request, bc_server_data_t * response) {

  if (client == NULL || client->bus_name == NULL || client->client_name == NULL ||
      request == NULL || strlen(client->client_name) >= BROADCAST_PEER_ID_SIZE) {
    return -1;
  }

  char full_bus_name[256];
  snprintf(
    full_bus_name,
    sizeof(full_bus_name),
    BROADCAST_BUS_NAME_TEMPLATE,
    client->bus_name
  );

  broadcast_message_t payload;
  payload.msg_type = BC_MSG_TYPE_CLIENT_REQUEST;
  snprintf(payload.peer_id, sizeof(payload.peer_id), "%s", client->client_name);
  memcpy(
    payload.data.client_request.request, 
    request->request, 
    sizeof(payload.data.client_request.request));

  for (int attempt = 0; attempt < REQUEST_MAX_ATTEMPTS; attempt++) {
    pthread_mutex_lock(&client->response_mutex);
    client->wait_for_resp = 1;
    client->response_available = 0;

    printf("DEBUG: Sending client request to broadcast server on bus: %s (attempt %d)\n", full_bus_name, attempt + 1);
    const bkk_dbus_err_t send_stat = bkk_dbus_send_signal(
      full_bus_name,
      &payload,
      sizeof(broadcast_message_t));

    if (send_stat != bkk_dbus_err_none) {
      fprintf(stderr, "Error sending signal: %d\n", send_stat);
      client->wait_for_resp = 0;
      pthread_mutex_unlock(&client->response_mutex);
      return -1;
    }

    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += RESPONSE_TIMEOUT_SECONDS;

    while (client->wait_for_resp) {
      const int wait_status = pthread_cond_timedwait(
        &client->response_received, &client->response_mutex, &deadline);
      if (wait_status == ETIMEDOUT) {
        client->wait_for_resp = 0;
        break;
      }
    }

    if (client->response_available) {
      if (response) {
        printf("DEBUG: Received response from broadcast server on bus: %s\n", full_bus_name);
        memcpy(response, &client->last_response, sizeof(bc_server_data_t));
      }
      pthread_mutex_unlock(&client->response_mutex);
      return 0;
    }
    pthread_mutex_unlock(&client->response_mutex);
  }

  printf("DEBUG: error: timeout for bus_name=%s\n", full_bus_name);
  fprintf(stderr, "Timed out waiting for broadcast server response\n");
  return -1;
}