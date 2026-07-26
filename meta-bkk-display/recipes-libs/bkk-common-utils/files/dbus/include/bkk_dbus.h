#ifndef BKK_DBUS_H
#define BKK_DBUS_H

#include "bkk_dbus_pub_types.h"
#include <unistd.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*bkk_dbus_listener_sig_hdl_t)(
  const char* sigvalue, size_t sigvalue_len, void* user_data);

typedef struct {
  int foo; 
  bkk_dbus_listener_sig_hdl_t sig_handler;
  void* user_data;
  pthread_t thread;
  void * conn; 
  char bus_name[256];
} bkk_dbus_listener_t;


bkk_dbus_err_t bkk_dbus_init_listener(
  const char * const bus_name, 
  bkk_dbus_listener_t* clt, 
  bkk_dbus_listener_sig_hdl_t sig_handler, 
  void* user_data);

bkk_dbus_err_t bkk_dbus_send_signal(
    const char * const bus_name, void *payload, size_t payload_size);

bkk_dbus_err_t bkk_dbus_deinit_listener(bkk_dbus_listener_t* clt);





#ifdef __cplusplus
}
#endif

#endif