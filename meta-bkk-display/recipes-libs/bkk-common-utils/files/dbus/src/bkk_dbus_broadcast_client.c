#include "bkk_dbus_broadcast_client.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define RESPONSE_TIMEOUT_SECONDS 1
#define REQUEST_MAX_ATTEMPTS 3

pthread_mutex_t response_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t response_received = PTHREAD_COND_INITIALIZER;
int wait_for_resp = 0;
int response_available = 0;
bc_server_data_t last_response;




static int server_response_handler(const char* sigvalue, size_t sigvalue_len, void* user_data) {
  if (sigvalue_len < sizeof(broadcast_message_t)) {
    fprintf(stderr, "Received signal is too small to contain a valid broadcast_message_t\n");
    return -1;
  }

  const broadcast_message_t* msg = (const broadcast_message_t*)sigvalue;
  const bc_client_t *client = (const bc_client_t*)user_data;

  if(msg->msg_type == BC_MSG_TYPE_SERVER_DATA) {
    pthread_mutex_lock(&response_mutex);
    if(wait_for_resp) {
      memcpy(&last_response, &msg->data.server_data, sizeof(bc_server_data_t));
      response_available = 1;
      wait_for_resp = 0;
      pthread_cond_signal(&response_received);
      pthread_mutex_unlock(&response_mutex);
    }
    else {
      pthread_mutex_unlock(&response_mutex);
      client->server_response_handler(
        msg->data.server_data.server_data, 
        sizeof(msg->data.server_data.server_data), 
        client->user_data);

    }
  }
  else {
    // dont care 
  }

  return 0;
}


int init_broadcast_client(const char * bus_name, 
    bkk_dbus_listener_t* clt, bkk_dbus_listener_sig_hdl_t handler, 
    void* user_data, bc_client_t *client) {
  memset(clt, 0, sizeof(*clt));

  client->server_response_handler = handler;
  client->user_data = user_data;
  client->bus_name = bus_name;
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


int send_client_request(const char * bus_name, 
  const bc_client_request_t *request, bc_server_data_t * response) {

  char full_bus_name[256];
  snprintf(
    full_bus_name,
    sizeof(full_bus_name),
    BROADCAST_BUS_NAME_TEMPLATE,
    bus_name
  );

  broadcast_message_t payload;
  payload.msg_type = BC_MSG_TYPE_CLIENT_REQUEST;
  memcpy(
    payload.data.client_request.request, 
    request->request, 
    sizeof(payload.data.client_request.request));

  for (int attempt = 0; attempt < REQUEST_MAX_ATTEMPTS; attempt++) {
    pthread_mutex_lock(&response_mutex);
    wait_for_resp = 1;
    response_available = 0;
    pthread_mutex_unlock(&response_mutex);

    const bkk_dbus_err_t send_stat = bkk_dbus_send_signal(
      full_bus_name,
      &payload,
      sizeof(broadcast_message_t));

    if (send_stat != bkk_dbus_err_none) {
      fprintf(stderr, "Error sending signal: %d\n", send_stat);
      pthread_mutex_lock(&response_mutex);
      wait_for_resp = 0;
      pthread_mutex_unlock(&response_mutex);
      return -1;
    }

    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += RESPONSE_TIMEOUT_SECONDS;

    pthread_mutex_lock(&response_mutex);
    while (wait_for_resp) {
      const int wait_status = pthread_cond_timedwait(
        &response_received, &response_mutex, &deadline);
      if (wait_status == ETIMEDOUT) {
        wait_for_resp = 0;
        break;
      }
    }

    if (response_available) {
      if (response) {
        memcpy(response, &last_response, sizeof(bc_server_data_t));
      }
      pthread_mutex_unlock(&response_mutex);
      return 0;
    }
    pthread_mutex_unlock(&response_mutex);
  }

  fprintf(stderr, "Timed out waiting for broadcast server response\n");
  return -1;
}