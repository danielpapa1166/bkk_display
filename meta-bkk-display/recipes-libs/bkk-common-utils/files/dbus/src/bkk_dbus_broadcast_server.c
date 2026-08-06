#include "bkk_dbus_broadcast_server.h"
#include <stdio.h>
#include <string.h>

static pthread_key_t request_server_key;
static pthread_once_t request_server_key_once = PTHREAD_ONCE_INIT;

static void create_request_server_key(void) {
  (void)pthread_key_create(&request_server_key, NULL);
}


static int client_request_dispather(const char* sigvalue, size_t sigvalue_len, void* user_data) {

  bc_server_t *server = (bc_server_t*)user_data;

  if (sigvalue_len < sizeof(broadcast_message_t)) {
    fprintf(stderr, "Received signal is too small to contain a valid broadcast_message_t\n");
    return -1;
  }

  const broadcast_message_t* msg = (const broadcast_message_t*)sigvalue;

  if(msg->msg_type == BC_MSG_TYPE_CLIENT_REQUEST) {
    if (memchr(msg->peer_id, '\0', sizeof(msg->peer_id)) == NULL) {
      fprintf(stderr, "Received client request with an invalid peer ID\n");
      return -1;
    }

    snprintf(
      server->request_peer_id,
      sizeof(server->request_peer_id),
      "%s",
      msg->peer_id);
    pthread_once(&request_server_key_once, create_request_server_key);
    pthread_setspecific(request_server_key, server);
    if (server->client_request_handler) {
      server->client_request_handler(
        msg->data.client_request.request,
        sizeof(msg->data.client_request.request),
        server->user_data);
    }
    pthread_setspecific(request_server_key, NULL);
    server->request_peer_id[0] = '\0';
  }
  else {
    // dont care 
  }

  return 0;
}


int init_broadcast_server(const char * bus_name, const char * server_name,
    bkk_dbus_listener_t* clt, bkk_dbus_listener_sig_hdl_t handler, 
    void* user_data, bc_server_t *server) {
  memset(clt, 0, sizeof(*clt));
  memset(server, 0, sizeof(*server));

  server->client_request_handler = handler;
  server->user_data = user_data;
  server->bus_name = bus_name;
  server->server_name = server_name;

  char BROADCAST_BUS_NAME[256];
  snprintf(
    BROADCAST_BUS_NAME, 
    sizeof(BROADCAST_BUS_NAME), 
    BROADCAST_BUS_NAME_TEMPLATE, 
    bus_name);

  bkk_dbus_err_t init_stat = bkk_dbus_init_listener(
    BROADCAST_BUS_NAME,
    clt,
    client_request_dispather,
    server
  );

  if (init_stat != bkk_dbus_err_none) {
    fprintf(stderr, "Error initializing broadcast server: %d\n", init_stat);
    return 1;
  }

  return 0;
}


int serve_data(bc_server_t *server, bc_server_data_t *server_data) {
  if (server == NULL || server_data == NULL || server->server_name == NULL ||
      strlen(server->server_name) >= BROADCAST_PEER_ID_SIZE) {
    return -1;
  }

  char bus_name[256];
  snprintf(
    bus_name,
    sizeof(bus_name),
    BROADCAST_BUS_NAME_TEMPLATE,
    server->bus_name
  );

  broadcast_message_t payload;
  payload.msg_type = BC_MSG_TYPE_SERVER_DATA;
  snprintf(
    payload.peer_id,
    sizeof(payload.peer_id),
    "%s",
    pthread_getspecific(request_server_key) == server ? server->request_peer_id : "");
  memcpy(
    &payload.data.server_data, 
    server_data, 
    sizeof(bc_server_data_t)
  );

  bkk_dbus_err_t send_signal_stat = bkk_dbus_send_signal(
    bus_name,
    &payload,
    sizeof(broadcast_message_t)
  );

  if (send_signal_stat != bkk_dbus_err_none) {
    fprintf(stderr, "Error sending signal: %d\n", send_signal_stat);
    return -1;
  }

  return 0;
}
