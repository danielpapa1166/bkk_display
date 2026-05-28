#ifndef AM_SUPERVISOR_H
#define AM_SUPERVISOR_H

#include "am_types.h"
#include <pthread.h>


typedef struct {
  app_config_list_t * app_config_list;
  app_info_list_t * app_info_list;
} supervisor_args_t;


void * supervisor_thread(void * args);


#endif // AM_SUPERVISOR_H