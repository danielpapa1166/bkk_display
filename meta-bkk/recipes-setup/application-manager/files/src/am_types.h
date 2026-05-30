#ifndef AM_TYPES_H
#define AM_TYPES_H

#include <string.h>
typedef enum {
  BOOT_MODE_UNDEFINED,
  BOOT_MODE_WIFI_CONFIG, 
  BOOT_MODE_API_CONFIG, 
  BOOT_MODE_NORMAL
} boot_mode_t;


static const inline char * boot_mode_to_string(boot_mode_t mode) {
  switch (mode) {
    case BOOT_MODE_WIFI_CONFIG:
      return "WIFI_CONFIG";
    case BOOT_MODE_API_CONFIG:
      return "API_CONFIG";
    case BOOT_MODE_NORMAL:
      return "NORMAL";
    case BOOT_MODE_UNDEFINED:
      return "UNDEFINED";
    default:
      return "UNKNOWN_BOOT_MODE";
  }
}

typedef enum {
  APP_STATUS_NOT_STARTED,
  APP_STATUS_NOT_IN_THIS_PHASE,
  APP_STATUS_RUNNING,
  APP_STATUS_EXITED, 
  APP_STATUS_KILLED, 
  APP_STATUS_FAILED, 
  APP_STATUS_OTHER_ERROR
} app_status_enum_t;


typedef struct {
  char * name; 
  int pid; 
  app_status_enum_t status;
  int exit_code;
} app_info_t;

typedef struct {
  app_info_t * app;
  int num_apps;
} app_info_list_t;


typedef struct {
  char * name;
  char * binary;
  char ** args;
  int num_args;
  char ** phases;
  int num_phases;
  char * after_started;
  char * after_exited;
  char * folder;
  char ** env; 
  int num_env; 
  app_info_t * info; 
} app_config_t;


typedef struct {
  app_config_t * app;
  int num_apps;
} app_config_list_t;



static int find_app_by_name(
    app_info_t * app_infos, int num_apps, const char * name) {

  for (int i = 0; i < num_apps; i++) {
    if (app_infos[i].name != NULL && strcmp(app_infos[i].name, name) == 0) {
      return i;
    }
  }
  return -1;
}

#endif // AM_TYPES_H