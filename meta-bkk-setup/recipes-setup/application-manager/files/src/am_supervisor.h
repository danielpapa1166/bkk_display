#ifndef AM_SUPERVISOR_H
#define AM_SUPERVISOR_H

#include "am_types.h"
#include <pthread.h>

typedef struct {
  app_info_t * app_infos; 
  int num_apps;
  pthread_mutex_t * lock;
} supervisor_args_t;


void * supervisor_thread(void * args);


#endif // AM_SUPERVISOR_H