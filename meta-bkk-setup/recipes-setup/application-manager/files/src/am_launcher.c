#include "am_launcher.h"
#include "am_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int launch_app(app_config_t * app, app_info_t * app_info) {

  app_info->name = strdup(app->name);

  // Build a NULL-terminated argv: [binary, arg0, arg1, ..., NULL]
  // argv[0] must be the program name (POSIX requirement).
  int full_argc = 1 + app->num_args + 1; // binary + args + NULL sentinel
  char ** argv = (char **)malloc(sizeof(char *) * full_argc);
  if (!argv) {
    app_info->pid = -1;
    app_info->status = APP_STATUS_FAILED;
    return -1;
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
    return pid;
  }

  // fork failed
  app_info->pid = -1;
  app_info->status = APP_STATUS_FAILED;
  return -1;
}