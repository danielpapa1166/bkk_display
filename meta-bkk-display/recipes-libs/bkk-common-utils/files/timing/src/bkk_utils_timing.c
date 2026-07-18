#include "bkk_utils_timing.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/eventfd.h>

static int is_nsec_valid(int nsec) {
  return nsec >= 0 && nsec < 1000000000;
}


timer_error_t bkk_setup_timer(timer_config_t * const config) {
  if (config == NULL) {
    return TIMER_ERROR_INVALID_CONFIG;
  }

  if (config->cyclic_expiration_sec < 0 || config->initial_expiration_sec < 0
      || !is_nsec_valid(config->cyclic_expiration_nsec)
      || !is_nsec_valid(config->initial_expiration_nsec)) {
    return TIMER_ERROR_INVALID_CONFIG;
  }

  config->timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
  if (config->timer_fd < 0) {
    return TIMER_ERROR_CREATE_FAILED;
  }

  struct itimerspec timerSpec;
  timerSpec.it_interval.tv_sec = config->cyclic_expiration_sec; // Interval for periodic timer
  timerSpec.it_interval.tv_nsec = config->cyclic_expiration_nsec;
  timerSpec.it_value.tv_sec = config->initial_expiration_sec; // Initial expiration
  timerSpec.it_value.tv_nsec = config->initial_expiration_nsec;

  const int res = timerfd_settime(
    config->timer_fd, 
    0, 
    &timerSpec, 
    NULL);

  if (res < 0) {
    (void)close(config->timer_fd);
    config->timer_fd = -1;
    return TIMER_ERROR_SET_FAILED;
  }
  
  FD_ZERO(&config->readfds);
  FD_SET(config->timer_fd, &config->readfds);
  return TIMER_ERROR_NONE;
}


timer_error_t bkk_wait_on_timer(timer_config_t * const config) {
  if (config == NULL || config->timer_fd < 0) {
    return TIMER_ERROR_INVALID_CONFIG;
  }

  int res = -1;
  while (1) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(config->timer_fd, &readfds);

    res = select(
      config->timer_fd + 1,
      &readfds,
      NULL,
      NULL,
      NULL);

    if (res < 0 && errno == EINTR) {
      continue;
    }
    break;
  }

  if (res < 0) {
    return TIMER_ERROR_SELECT_FAILED;
  }

  uint64_t expirations = 0;
  while (1) {
    const ssize_t n = read(config->timer_fd, &expirations, sizeof(expirations));
    if (n == (ssize_t)sizeof(expirations)) {
      break;
    }
    if (n < 0 && errno == EINTR) {
      continue;
    }
    return TIMER_ERROR_READ_FAILED;
  }

  return TIMER_ERROR_NONE;
}

static void * timer_thread_func(void * arg) {
  timer_thread_ctx_t * const thread_ctx = (timer_thread_ctx_t * const)arg;

  while (1) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(thread_ctx->config.timer_fd, &readfds);
    FD_SET(thread_ctx->stop_fd, &readfds);

    const int max_fd = thread_ctx->config.timer_fd > thread_ctx->stop_fd
      ? thread_ctx->config.timer_fd
      : thread_ctx->stop_fd;

    const int select_res = select(max_fd + 1, &readfds, NULL, NULL, NULL);
    if (select_res < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }

    if (FD_ISSET(thread_ctx->stop_fd, &readfds)) {
      // stop signal was received, exit loop: 
      uint64_t stop_signal = 0;
      (void)read(thread_ctx->stop_fd, &stop_signal, sizeof(stop_signal));
      break;
    }

    if (FD_ISSET(thread_ctx->config.timer_fd, &readfds)) {
      // timer elapsed: 
      uint64_t expirations = 0;
      const ssize_t n = read(
        thread_ctx->config.timer_fd,
        &expirations,
        sizeof(expirations));

      if (n != (ssize_t)sizeof(expirations)) {
        break;
      }

      (void) thread_ctx->callback(thread_ctx->arg);

    }
  }

  return NULL;
}


timer_error_t bkk_setup_timer_with_callback(
    timer_thread_ctx_t * const thread_ctx) {

  if (thread_ctx == NULL || thread_ctx->callback == NULL) {
    return TIMER_ERROR_INVALID_CONFIG;
  }

  thread_ctx->is_running = false;
  thread_ctx->thread_created = false;
  thread_ctx->thread_joined = false;
  thread_ctx->stop_fd = -1;
  thread_ctx->config.timer_fd = -1;

  const timer_error_t setup_res = bkk_setup_timer(&thread_ctx->config);
  if (setup_res != TIMER_ERROR_NONE) {
    return setup_res;
  }

  thread_ctx->stop_fd = eventfd(0, EFD_CLOEXEC);
  if (thread_ctx->stop_fd < 0) {
    (void)bkk_cleanup_timer(&thread_ctx->config);
    return TIMER_ERROR_CREATE_FAILED;
  }

  const int thread_create_res = pthread_create(
    &thread_ctx->thread,
    NULL,
    timer_thread_func,
    thread_ctx);

  if (thread_create_res != 0) {
    (void)close(thread_ctx->stop_fd);
    thread_ctx->stop_fd = -1;
    (void)bkk_cleanup_timer(&thread_ctx->config);
    return TIMER_ERROR_THREAD_CREATE_FAILED;
  }

  thread_ctx->thread_created = true;
  thread_ctx->is_running = true;
  thread_ctx->thread_id = (unsigned long)thread_ctx->thread;
  return TIMER_ERROR_NONE;

}

timer_error_t bkk_stop_timer_with_callback(
    timer_thread_ctx_t * const thread_ctx) {
  if (thread_ctx == NULL) {
    return TIMER_ERROR_INVALID_CONFIG;
  }

  if (!thread_ctx->thread_created || thread_ctx->thread_joined
      || thread_ctx->stop_fd < 0) {
    return TIMER_ERROR_NONE;
  }

  const uint64_t stop_signal = 1;
  const ssize_t n = write(
    thread_ctx->stop_fd,
    &stop_signal,
    sizeof(stop_signal));

  if (n != (ssize_t)sizeof(stop_signal)) {
    return TIMER_ERROR_STOP_SIGNAL_FAILED;
  }

  return TIMER_ERROR_NONE;
}

timer_error_t bkk_join_timer_with_callback(
    timer_thread_ctx_t * const thread_ctx) {
  if (thread_ctx == NULL) {
    return TIMER_ERROR_INVALID_CONFIG;
  }

  if (!thread_ctx->thread_created || thread_ctx->thread_joined) {
    return TIMER_ERROR_NONE;
  }

  const int join_res = pthread_join(thread_ctx->thread, NULL);
  if (join_res != 0) {
    return TIMER_ERROR_THREAD_JOIN_FAILED;
  }

  thread_ctx->is_running = false;
  thread_ctx->thread_joined = true;
  return TIMER_ERROR_NONE;
}

timer_error_t bkk_cleanup_timer(timer_config_t * const config) {
  if (config == NULL) {
    return TIMER_ERROR_INVALID_CONFIG;
  }

  if (config->timer_fd >= 0) {
    if (close(config->timer_fd) < 0) {
      return TIMER_ERROR_READ_FAILED;
    }
    config->timer_fd = -1;
  }

  FD_ZERO(&config->readfds);
  return TIMER_ERROR_NONE;
}

timer_error_t bkk_cleanup_timer_with_callback(
    timer_thread_ctx_t * const thread_ctx) {
  if (thread_ctx == NULL) {
    return TIMER_ERROR_INVALID_CONFIG;
  }

  timer_error_t res = bkk_stop_timer_with_callback(thread_ctx);
  if (res != TIMER_ERROR_NONE) {
    return res;
  }

  res = bkk_join_timer_with_callback(thread_ctx);
  if (res != TIMER_ERROR_NONE) {
    return res;
  }

  if (thread_ctx->stop_fd >= 0) {
    if (close(thread_ctx->stop_fd) < 0) {
      return TIMER_ERROR_READ_FAILED;
    }
    thread_ctx->stop_fd = -1;
  }

  return bkk_cleanup_timer(&thread_ctx->config);
}
