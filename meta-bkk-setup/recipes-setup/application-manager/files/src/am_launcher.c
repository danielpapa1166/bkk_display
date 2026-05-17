#include "am_launcher.h"
#include "am_types.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


static int check_app_valid_for_boot_mode(boot_mode_t boot_mode, app_config_t * app) {
  for (int i = 0; i < app->num_phases; i++) {
    if (strcmp(app->phases[i], boot_mode_to_string(boot_mode)) == 0) {
      return 1;
    }
  }
  return 0;
}

static char ** build_argv(app_config_t * app) {
  int full_argc = 1 + app->num_args + 1; // binary + args + NULL sentinel
  char ** argv = (char **)malloc(sizeof(char *) * full_argc);
  if (!argv) {
    return NULL;
  }
  argv[0] = app->binary;
  for (int i = 0; i < app->num_args; i++) {
    argv[1 + i] = app->args[i];
  }
  argv[full_argc - 1] = NULL;
  return argv;
}


static char ** build_envp(app_config_t * app) {
  if (app->num_env == 0) {
    return NULL;
  }
  char ** envp = (char **)malloc(sizeof(char *) * (app->num_env + 1));
  if (!envp) {
    return NULL;
  }
  for (int i = 0; i < app->num_env; i++) {
    envp[i] = app->env[i];
  }
  envp[app->num_env] = NULL;
  return envp;
}




const char * launch_status_to_string(launch_status_t status) {
  switch (status) {
    case LAUNCH_OK:
      return "OK";
    case LAUNCH_OK_NOT_LAUNCHED: 
      return "OK_NOT_LAUNCHED"; 
    case LAUNCH_ERR_INVALID_CONFIG:
      return "ERR_INVALID_CONFIG";
    case LAUNCH_ERR_INTERNAL:
      return "ERR_INTERNAL";
    case LAUNCH_ERR_FOLDER:
      return "ERR_FOLDER";
    case LAUNCH_ERR_FORK:
      return "ERR_FORK";
    case LAUNCH_ERR_EXEC:
      return "ERR_EXEC";
    default:
      return "UNKNOWN_STATUS";
  }
}


launch_status_t launch_app(boot_mode_t boot_mode, 
    app_config_t * app, app_info_list_t * app_info) {

  // todo: check app info for dependencies of the given app 

  app->info->name = strdup(app->name);
  if (!app->info->name) {
    app->info->pid = -1;
    app->info->status = APP_STATUS_FAILED;
    return LAUNCH_ERR_INTERNAL;
  }

  if(!check_app_valid_for_boot_mode(boot_mode, app)) {
    app->info->pid = -1;
    app->info->status = APP_STATUS_NOT_IN_THIS_PHASE;
    return LAUNCH_OK_NOT_LAUNCHED;
  }

  if (app->folder != NULL) {
    const int mkdir_result = mkdir(app->folder, 0755);
    if (mkdir_result != 0 && errno != EEXIST) {
      fprintf(stderr, 
        "am_launcher: mkdir failed for '%s': %s\n", app->folder, strerror(errno));
      free(app->info->name);
      app->info->name = NULL;
      app->info->pid = -1;
      app->info->status = APP_STATUS_FAILED;
      return LAUNCH_ERR_FOLDER;
    }
  }

  // Build a NULL-terminated argv: [binary, arg0, arg1, ..., NULL]
  // argv[0] must be the program name (POSIX requirement).
  char ** argv = build_argv(app);
  if (!argv) {
    fprintf(stderr, 
      "am_launcher: failed to build argv for '%s'\n", app->name);
    app->info->pid = -1;
    app->info->status = APP_STATUS_FAILED;
    return LAUNCH_ERR_INTERNAL;
  }

  char ** envp = build_envp(app);
  if (app->num_env > 0 && !envp) {
    fprintf(stderr, 
      "am_launcher: failed to build envp for '%s'\n", app->name);
    free(argv);
    app->info->pid = -1;
    app->info->status = APP_STATUS_FAILED;
    return LAUNCH_ERR_INTERNAL;
  }

  pid_t pid = fork();
  if (pid == 0) {
    // child process

    if(app->env != NULL) {
      // if env vars are provided, use execve which allows passing envp
      execve(app->binary, argv, envp);
    }
    else {
      execv(app->binary, argv);
    }
    // execv only returns on failure
    fprintf(stderr, "am_launcher: execv failed for '%s'\n", app->binary);
    _exit(1);
  }

  free(argv);

  if (pid > 0) {
    // parent process: record child PID
    app->info->pid = pid;
    app->info->status = APP_STATUS_RUNNING;
    return LAUNCH_OK;
  }

  // fork failed
  app->info->pid = -1;
  app->info->status = APP_STATUS_FAILED;
  return LAUNCH_ERR_FORK;
}