#ifndef AM_TYPES_H
#define AM_TYPES_H

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


typedef struct {
  char * name;
  char * binary;
  char ** args;
  int num_args;
  char ** phases;
  int num_phases;
  char * after;
  char * folder;
  char ** env; 
  int num_env; 
} app_config_t;


typedef struct {
  app_config_t * apps;
  int num_apps;
} app_config_list_t;


typedef enum {
  APP_STATUS_NOT_STARTED,
  APP_STATUS_NOT_IN_THIS_PHASE,
  APP_STATUS_RUNNING,
  APP_STATUS_EXITED, 
  APP_STATUS_KILLED, 
  APP_STATUS_FAILED
} app_status_enum_t;

typedef struct {
  char * name; 
  int pid; 
  app_status_enum_t status;
  int exit_code;
} app_info_t;

#endif // AM_TYPES_H