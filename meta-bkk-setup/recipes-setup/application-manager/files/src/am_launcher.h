#ifndef AM_LAUNCHER_H
#define AM_LAUNCHER_H

#include "am_types.h"

typedef enum {
  LAUNCH_OK,
  LAUNCH_OK_NOT_LAUNCHED,
  LAUNCH_OK_DELAYED_LAUNCH,
  LAUNCH_ERR_INVALID_CONFIG,
  LAUNCH_ERR_INTERNAL, 
  LAUNCH_ERR_FOLDER, 
  LAUNCH_ERR_FORK,
  LAUNCH_ERR_EXEC
} launch_status_t;
const char * launch_status_to_string(launch_status_t status);
launch_status_t launch_app(boot_mode_t boot_mode, 
  app_config_t * app, app_info_list_t * app_info);

#endif // AM_LAUNCHER_H