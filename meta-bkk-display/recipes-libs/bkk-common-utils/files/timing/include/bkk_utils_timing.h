#ifndef BKK_UTILS_TIMING_H
#define BKK_UTILS_TIMING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/select.h>

typedef enum {
  TIMER_ERROR_NONE = 0,
  TIMER_ERROR_INVALID_CONFIG,
  TIMER_ERROR_CREATE_FAILED,
  TIMER_ERROR_SET_FAILED,
  TIMER_ERROR_SELECT_FAILED,
  TIMER_ERROR_READ_FAILED,
  TIMER_ERROR_THREAD_CREATE_FAILED,
  TIMER_ERROR_THREAD_JOIN_FAILED,
  TIMER_ERROR_STOP_SIGNAL_FAILED
} timer_error_t; 


typedef struct {
  int timer_fd;
  int cyclic_expiration_sec;
  int cyclic_expiration_nsec;
  int initial_expiration_sec;
  int initial_expiration_nsec;
  fd_set readfds;
} timer_config_t;

typedef struct {
  timer_config_t config;
  int (*callback)(void * arg);
  void * arg;
  int stop_fd;
  bool is_running;
  bool thread_created;
  bool thread_joined;
  pthread_t thread;
  unsigned long thread_id;
} timer_thread_ctx_t;

typedef int (*timer_callback_t)(void * arg);


timer_error_t bkk_setup_timer(timer_config_t * const config);
timer_error_t bkk_wait_on_timer(timer_config_t * const config);

timer_error_t bkk_setup_timer_with_callback(
  timer_thread_ctx_t * const thread_ctx);

timer_error_t bkk_stop_timer_with_callback(
  timer_thread_ctx_t * const thread_ctx);

timer_error_t bkk_join_timer_with_callback(
  timer_thread_ctx_t * const thread_ctx);

timer_error_t bkk_cleanup_timer(timer_config_t * const config);

timer_error_t bkk_cleanup_timer_with_callback(
  timer_thread_ctx_t * const thread_ctx);


#ifdef __cplusplus
}
#endif 

#endif // BKK_UTILS_TIMING_H