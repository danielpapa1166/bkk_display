#include "am_launcher.h"
#include "am_types.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

const char * launch_status_to_string(launch_status_t status) {
  switch (status) {
    case LAUNCH_OK:
      return "LAUNCH_OK";
    case LAUNCH_ERR_INVALID_CONFIG:
      return "LAUNCH_ERR_INVALID_CONFIG";
    case LAUNCH_ERR_INTERNAL:
      return "LAUNCH_ERR_INTERNAL";
    case LAUNCH_ERR_FOLDER:
      return "LAUNCH_ERR_FOLDER";
    case LAUNCH_ERR_FORK:
      return "LAUNCH_ERR_FORK";
    case LAUNCH_ERR_EXEC:
      return "LAUNCH_ERR_EXEC";
    default:
      return "UNKNOWN_STATUS";
  }
}


launch_status_t launch_app(app_config_t * app, app_info_t * app_info) {

  app_info->name = strdup(app->name);
  if (!app_info->name) {
    app_info->pid = -1;
    app_info->status = APP_STATUS_FAILED;
    return LAUNCH_ERR_INTERNAL;
  }

  if (app->folder != NULL) {
    const int mkdir_result = mkdir(app->folder, 0755);
    if (mkdir_result != 0 && errno != EEXIST) {
      fprintf(stderr, 
        "am_launcher: mkdir failed for '%s': %s\n", app->folder, strerror(errno));
      free(app_info->name);
      app_info->name = NULL;
      app_info->pid = -1;
      app_info->status = APP_STATUS_FAILED;
      return LAUNCH_ERR_FOLDER;
    }
  }

  // Build a NULL-terminated argv: [binary, arg0, arg1, ..., NULL]
  // argv[0] must be the program name (POSIX requirement).
  int full_argc = 1 + app->num_args + 1; // binary + args + NULL sentinel
  char ** argv = (char **)malloc(sizeof(char *) * full_argc);
  if (!argv) {
    app_info->pid = -1;
    app_info->status = APP_STATUS_FAILED;
    return LAUNCH_ERR_INTERNAL;
  }
  argv[0] = app->binary;
  for (int i = 0; i < app->num_args; i++) {
    argv[1 + i] = app->args[i];
  }
  argv[full_argc - 1] = NULL;

  pid_t pid = fork();
  if (pid == 0) {
    // child process
    execv(app->binary, argv);
    // execv only returns on failure
    fprintf(stderr, "am_launcher: execv failed for '%s'\n", app->binary);
    _exit(1);
  }

  free(argv);

  if (pid > 0) {
    // parent process: record child PID
    app_info->pid = pid;
    app_info->status = APP_STATUS_RUNNING;
    return LAUNCH_OK;
  }

  // fork failed
  app_info->pid = -1;
  app_info->status = APP_STATUS_FAILED;
  return LAUNCH_ERR_FORK;
}