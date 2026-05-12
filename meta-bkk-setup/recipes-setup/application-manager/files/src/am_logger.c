#include "am_logger.h"

#include <stdlib.h>
#include <string.h>
#include "rbuflogd/producer.h"

static const char * logger_name = "AM";
static rbuflogd_producer_t *global_logger_producer = NULL;


int init_logger(void) {
  global_logger_producer = malloc(sizeof(rbuflogd_producer_t));
  if(global_logger_producer == NULL) {
    return -1;
  }

  global_logger_producer->state = NULL;

  const int open_res = rbuflogd_producer_open(
    global_logger_producer, 
    logger_name);

  if(open_res != 0) {
    free(global_logger_producer);
    global_logger_producer = NULL;
    return -1;
  }

  return 0; 
}


void cleanup_logger(void) {
  if(global_logger_producer != NULL) {
    rbuflogd_producer_close(global_logger_producer);
    free(global_logger_producer);
    global_logger_producer = NULL;
  }
}


void rename_logger(const char *new_name, int length) {
  if(global_logger_producer == NULL) {
    return;
  }

  if (global_logger_producer->state == NULL) {
    return;
  }

  if (length > RBUF_PROD_ID_MAX_LEN) {
    length = RBUF_PROD_ID_MAX_LEN;
  }

  memcpy(
    global_logger_producer->producer_name, 
    new_name, 
    (size_t)length);
}


int log_debug(const char * const category, const char * const message) {
  if(global_logger_producer == NULL) {
    return -1;
  }

  if (global_logger_producer->state == NULL) {
    return -1;
  }

  const int log_res = rbuflogd_producer_log(
    global_logger_producer, 
    RBUF_LOG_LEVEL_DEBUG, 
    category, 
    message);

  return log_res;
}

int log_info(const char * const category, const char * const message) {
  if(global_logger_producer == NULL) {
    return -1;
  }

  if (global_logger_producer->state == NULL) {
    return -1;
  }

  const int log_res = rbuflogd_producer_log(
    global_logger_producer, 
    RBUF_LOG_LEVEL_INFO, 
    category, 
    message);

  return log_res;
}

int log_warning(const char * const category, const char * const message) {
  if(global_logger_producer == NULL) {
    return -1;
  }

  if (global_logger_producer->state == NULL) {
    return -1;
  }

  const int log_res = rbuflogd_producer_log(
    global_logger_producer, 
    RBUF_LOG_LEVEL_WARNING, 
    category, 
    message);

  return log_res;
}

int log_error(const char * const category, const char * const message) {
  if(global_logger_producer == NULL) {
    return -1;
  }

  if (global_logger_producer->state == NULL) {
    return -1;
  }

  const int log_res = rbuflogd_producer_log(
    global_logger_producer, 
    RBUF_LOG_LEVEL_ERROR, 
    category, 
    message);

  return log_res;
}