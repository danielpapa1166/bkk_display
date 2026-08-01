#include "bkk_dbus.h"
#include "bkk_dbus_pub_types.h"
#include <stdint.h>
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static pthread_once_t dbus_thread_init_once = PTHREAD_ONCE_INIT;
static int dbus_thread_init_result = 0;

static void initialize_dbus_thread_support(void) {
  dbus_thread_init_result = dbus_threads_init_default();
}

static int ensure_dbus_thread_support(void) {
  pthread_once(&dbus_thread_init_once, initialize_dbus_thread_support);
  return dbus_thread_init_result;
}


static void * bkk_dbus_listen_thread(void *arg) {
  bkk_dbus_listener_t* clt = (bkk_dbus_listener_t*)arg;

  DBusMessage* msg;
  DBusMessageIter args;
  DBusConnection* conn = (DBusConnection*)clt->conn;
  const char* bus_name = clt->bus_name;
  DBusError err;
  int ret;
  char* sigvalue;
  size_t sigvalue_len;

  // initialise the errors
  dbus_error_init(&err);  


  while (1) {
    // non blocking read of the next available message
    dbus_connection_read_write(conn, 0);
    msg = dbus_connection_pop_message(conn);

    // loop again if we haven't read a message
    if (NULL == msg) {
      usleep(100000);
      continue;
    }

    // check if the message is a signal from the correct interface and with the correct name
    if (dbus_message_is_signal(msg, bus_name, "Test")) {
      const dbus_bool_t has_args = dbus_message_iter_init(msg, &args);

      if (has_args) {

        const int arg_type = dbus_message_iter_get_arg_type(&args);

        if(arg_type == DBUS_TYPE_STRING) {
          dbus_message_iter_get_basic(&args, &sigvalue);
          sigvalue_len = strlen(sigvalue);
        }
        else if(arg_type == DBUS_TYPE_ARRAY) {
          DBusMessageIter array_iter;
          dbus_message_iter_recurse(&args, &array_iter);

          const int element_type = dbus_message_iter_get_arg_type(&array_iter);

          if (element_type == DBUS_TYPE_BYTE) {
            uint8_t *byte_array;
            int array_len;
            dbus_message_iter_get_fixed_array(&array_iter, &byte_array, &array_len);
            sigvalue = (char*)byte_array;
            sigvalue_len = array_len;
          }
        }
        else {
          fprintf(stderr, "Signal has unsupported argument type: %c\n", arg_type);
        }

      }
      else {
        fprintf(stderr, "Signal has no arguments!\n");
      }

      // call the user-defined signal handler
      if (clt->sig_handler) {
        clt->sig_handler(sigvalue, sigvalue_len, clt->user_data);
      }
    }

    // free the message
    dbus_message_unref(msg);
  }

  return NULL;
}


bkk_dbus_err_t bkk_dbus_init_listener(
    const char * const bus_name, 
    bkk_dbus_listener_t* clt, 
    bkk_dbus_listener_sig_hdl_t sig_handler, 
    void* user_data) {

  if (!ensure_dbus_thread_support()) {
    fprintf(stderr, "Failed to initialize D-Bus thread support\n");
    return bkk_dbus_err_other;
  }

  clt->foo = 0;
  clt->sig_handler = sig_handler;
  clt->user_data = user_data;

  DBusMessage* msg;
  DBusMessageIter args;
  DBusConnection* conn;
  DBusError err;
  // initialise the errors
  dbus_error_init(&err);  

  // connect to the bus and check for errors
  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Connection Error (%s)\n", err.message);
    dbus_error_free(&err);
    return bkk_dbus_err_other;
  }
  if (NULL == conn) {
    return bkk_dbus_err_other;
  }

  char rule[256];
  snprintf(
    rule, 
    sizeof(rule), 
    "type='signal',interface='%s'", 
    bus_name);

  dbus_bus_add_match(
    conn, rule, &err); 
  dbus_connection_flush(conn);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Match Error (%s)\n", err.message);
    return bkk_dbus_err_other;
  }

  clt->conn = (void*)conn;
  snprintf(clt->bus_name, sizeof(clt->bus_name), "%s", bus_name);

  // create a thread to listen for signals
  const int thread_stat = pthread_create(
    &clt->thread, 
    NULL, 
    bkk_dbus_listen_thread, 
    (void*)clt);

  if(thread_stat != 0) {
    fprintf(stderr, "Error creating thread: %d\n", thread_stat);
    return bkk_dbus_err_other;
  }
  

  return bkk_dbus_err_none;
}


static bkk_dbus_err_t bkk_dbus_set_binary_payload(
    DBusMessage* msg, 
    void* payload, 
    size_t payload_size) {

  DBusMessageIter iter, array_iter;
 
  // Initialize message for append
  dbus_message_iter_init_append(msg, &iter);

  // Open an array 
  const dbus_bool_t open_success = dbus_message_iter_open_container(
    &iter, 
    DBUS_TYPE_ARRAY, 
    DBUS_TYPE_BYTE_AS_STRING, 
    &array_iter);

  if (!open_success) {
    fprintf(stderr, "Out Of Memory!\n");
    return bkk_dbus_err_other;
  }

  // Append the fixed array
  const dbus_bool_t append_success = dbus_message_iter_append_fixed_array(
    &array_iter, 
    DBUS_TYPE_BYTE, 
    &payload,
    payload_size);

  if (!append_success) {
    fprintf(stderr, "Out Of Memory!\n");
    return bkk_dbus_err_other;
  }

  // Close the container
  dbus_message_iter_close_container(&iter, &array_iter);

  return bkk_dbus_err_none;
}


bkk_dbus_err_t bkk_dbus_send_signal(
    const char * const bus_name, void *payload, size_t payload_size) {
  (void) bus_name; 

  if (!ensure_dbus_thread_support()) {
    fprintf(stderr, "Failed to initialize D-Bus thread support\n");
    return bkk_dbus_err_other;
  }

  DBusMessage* msg;
  DBusMessageIter args;
  DBusConnection* conn;
  DBusError err;
  dbus_uint32_t serial = 0;

  // initialise the error value
  dbus_error_init(&err);

  // connect to the DBUS system bus, and check for errors
  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Connection Error (%s)\n", err.message);
    dbus_error_free(&err);
    return bkk_dbus_err_other;
  }
  if (NULL == conn) {
    return bkk_dbus_err_other;
  }


  // create a signal & check for errors
  msg = dbus_message_new_signal(
    "/test/signal/Object", // object name of the signal
    bus_name, // interface name of the signal
    "Test"); // name of the signal

  if (NULL == msg)
  {
    fprintf(stderr, "Message Null\n");
    return bkk_dbus_err_other;
  }

  // append arguments onto signal
  bkk_dbus_err_t payload_stat = bkk_dbus_set_binary_payload(
    msg, 
    payload, 
    payload_size);

  // send the message and flush the connection
  if (!dbus_connection_send(conn, msg, &serial)) {
    fprintf(stderr, "Out Of Memory!\n");
    return bkk_dbus_err_other;
  }
  dbus_connection_flush(conn);

  // free the message
  dbus_message_unref(msg);

  return bkk_dbus_err_none;
}


bkk_dbus_err_t bkk_dbus_deinit_listener(bkk_dbus_listener_t* clt) {
  // cancel the listening thread
  pthread_cancel(clt->thread);
  pthread_join(clt->thread, NULL);

  // close the DBus connection
  if (clt->conn) {
    dbus_connection_unref(
        (DBusConnection*)clt->conn);
    clt->conn = NULL;
  }

  return bkk_dbus_err_none;
}


dbus_status_t check_dbus_connection(void) {
  DBusError err;
  DBusConnection* conn;
  int ret;
  // initialise the errors
  dbus_error_init(&err);
  // connect to the bus
  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) { 
    dbus_error_free(&err); 
    //dbus_connection_close(conn);
    return DBUS_NAME_REQUEST_FAILED;
  }
  if (NULL == conn) { 
    dbus_error_free(&err); 
    //dbus_connection_close(conn);
    return DBUS_CONNECTION_FAILED;
  }
  // request a name on the bus
  /*ret = dbus_bus_request_name(
    conn, 
    "test.method.server", 
    DBUS_NAME_FLAG_REPLACE_EXISTING, 
    &err);

  if (dbus_error_is_set(&err)) { 
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg), 
      "Failed to request D-Bus name: %s", err.message);
    log_error("DBus", err_msg);
    dbus_error_free(&err); 
    dbus_connection_close(conn);
    return DBUS_INACTIVE;
  }*/
  /*if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
    dbus_connection_close(conn);
    return DBUS_INACTIVE;
  }*/
  //dbus_connection_close(conn);
  return DBUS_ACTIVE;
}


dbus_status_t wait_for_dbus_connection(int timeout_s) {
  bool wait_forever = (timeout_s < 0);

  // check connection status: 
  dbus_status_t dbus_stat;

  do {
    dbus_stat = check_dbus_connection();
    if(dbus_stat != DBUS_ACTIVE) {
      sleep(1);
    }
  } while(dbus_stat != DBUS_ACTIVE && (wait_forever || timeout_s-- > 0));
  return dbus_stat;
}