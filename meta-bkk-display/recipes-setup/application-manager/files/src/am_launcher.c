#include "am_launcher.h"
#include "am_types.h"
#include "rbuflogd/logger.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char msg_buf[256];

static int check_app_valid_for_boot_mode(boot_mode_t boot_mode, app_config_t * app) {
  // obsolete function, as we now filter app configs by boot mode before passing to launcher
  // keeping this function in case we want to re-use it for more complex boot mode checks in the future
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


typedef struct {
  app_status_enum_t app_status;
  int exit_code;
} app_prereq_status_t; 


static app_prereq_status_t get_app_prereq_status(
    const char * const app_name, app_info_list_t * app_info) {
  const int app_id = find_app_by_name(
    app_info->app, 
    app_info->num_apps, 
    app_name);

  if (app_id < 0) {
    app_prereq_status_t status = {
      .app_status = APP_STATUS_OTHER_ERROR,
      .exit_code = -1
    };
    return status;
  }
  app_prereq_status_t status = {
    .app_status = app_info->app[app_id].status,
    .exit_code = app_info->app[app_id].exit_code
  };
  return status;
}


typedef enum {
  PREREQ_OK,
  PREREQ_STILL_RUNNING,
  PREREQ_FAILED, 
  PREREQ_INVALID_CONFIG,
  PREREQ_OTHER_ERROR
} prereq_status_t;

static prereq_status_t check_prerequisites(
    app_config_t * app, app_info_list_t * app_info) {

  const char * log_cat = "req_chk";
  const char * wait_for_exited = app->after_exited;
  const char * wait_for_started = app->after_started;

  if (wait_for_exited == NULL && wait_for_started == NULL) {
    return PREREQ_OK;
  }

  if (wait_for_exited != NULL) {
    const app_prereq_status_t prereq_exited
      = get_app_prereq_status(wait_for_exited, app_info);

    if (prereq_exited.app_status == APP_STATUS_OTHER_ERROR) {
      snprintf(
        msg_buf,
        sizeof(msg_buf),
        "check_prerequisites: app '%s' has invalid prerequisite app '%s'",
        app->name, wait_for_exited);
      log_warning(log_cat, msg_buf);
      return PREREQ_INVALID_CONFIG;
    }

    if (prereq_exited.app_status != APP_STATUS_EXITED) {
      return PREREQ_STILL_RUNNING;
    }

    if (prereq_exited.exit_code != 0) {
      return PREREQ_FAILED;
    }
  }

  if (wait_for_started != NULL) {
    const app_prereq_status_t prereq_started
      = get_app_prereq_status(wait_for_started, app_info);

    if (prereq_started.app_status == APP_STATUS_OTHER_ERROR) {
      snprintf(
        msg_buf,
        sizeof(msg_buf),
        "check_prerequisites: app '%s' has invalid prerequisite app '%s'",
        app->name, wait_for_started);
      log_warning(log_cat, msg_buf);
      return PREREQ_INVALID_CONFIG;
    }

    if (prereq_started.app_status != APP_STATUS_RUNNING) {
      return PREREQ_STILL_RUNNING;
    }
  }

  return PREREQ_OK;
}


const char * launch_status_to_string(launch_status_t status) {
  switch (status) {
    case LAUNCH_OK:
      return "OK";
    case LAUNCH_OK_NOT_LAUNCHED: 
      return "OK_NOT_LAUNCHED"; 
    case LAUNCH_OK_DELAYED_LAUNCH:
      return "OK_DELAYED_LAUNCH";
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


launch_status_t launch_app(app_config_t * app, app_info_list_t * app_info) {

  if(app->info->status != APP_STATUS_NOT_STARTED) {
    return LAUNCH_OK_NOT_LAUNCHED; 
  }

  const prereq_status_t prereq_status = check_prerequisites(app, app_info);
  if(prereq_status != PREREQ_OK) {
    if(prereq_status == PREREQ_INVALID_CONFIG) {
      return LAUNCH_ERR_INVALID_CONFIG;
    }
    else if (prereq_status == PREREQ_FAILED) {
      return LAUNCH_ERR_EXEC;
    }
    else {
      return LAUNCH_OK_DELAYED_LAUNCH;
    }
  }

  // no need to check boot mode, 
  // as only the filtered app configs 
  // for the current boot mode are passed to this function

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