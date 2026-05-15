#ifndef AM_TYPES_H
#define AM_TYPES_H

typedef struct {
  char * name;
  char * binary;
  char ** args;
  int num_args;
  char ** phases;
  int num_phases;
  char * after;
  char * folder;
} app_config_t;


typedef struct {
  app_config_t * apps;
  int num_apps;
} app_config_list_t;


typedef enum {
  APP_STATUS_NOT_STARTED,
  APP_STATUS_RUNNING,
  APP_STATUS_EXITED, 
  APP_STATUS_FAILED
} app_status_enum_t;

typedef struct {
  char * name; 
  int pid; 
  app_status_enum_t status;
} app_info_t;

#endif // AM_TYPES_H