#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include "rbuflogd/logger.h"

#include "am_config_parser.h"
#include "am_launcher.h"
#include "am_types.h"
#include "am_supervisor.h"
#include "am_boot_mode.h"

// ----------------------------------------------------------------------------
// internal helper functions
// ----------------------------------------------------------------------------
static app_config_list_t * filter_app_info_by_phase(
    const char * phase, const app_config_list_t * app_config_list);
static void init_app_config_struct(
  app_config_list_t * app_config_list, const app_info_list_t * app_info_list); 
static int create_supervisor_thread(
    app_config_list_t * app_config_list, app_info_list_t * app_info_list);

// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------

int main(int argc, char * argv[])
{
  // --- parse CLI ---
  am_cli_args_t cli_args;
  parse_status_t status = parse_cli(argc, argv, &cli_args);
  if (status != PARSE_OK) {
    fprintf(stderr, "am: invalid command line arguments\n");
    return 1;
  }

  // --- parse config file ---
  app_config_list_t * config_list = (app_config_list_t *)malloc(sizeof(app_config_list_t));
  status = parse_config(cli_args.config_path, config_list);
  if (status != PARSE_OK) {
    fprintf(stderr, "am: failed to parse config file\n");
    return 1;
  }

  const boot_mode_t boot_mode = determine_boot_mode(cli_args.boot_flags_dir);
  const char * boot_mode_str = boot_mode_to_string(boot_mode);
  fprintf(stderr, "am: determined boot mode: %s\n", boot_mode_str);
  
   app_config_list_t * config_list_filt = filter_app_info_by_phase(
    boot_mode_str, 
    config_list);

  free(config_list->app);
  free(config_list);

  config_list = config_list_filt;



  app_info_list_t app_info_list = {
    .app = (app_info_t *)calloc(
      config_list->num_apps, sizeof(app_info_t)),
    .num_apps = config_list->num_apps
  };

  if (app_info_list.app == NULL) {
    fprintf(stderr, "am: failed to allocate app_info array\n");
    return 1;
  }

  init_app_config_struct(config_list, &app_info_list);

  const int thread_stat = create_supervisor_thread(
    config_list, &app_info_list);

  if (thread_stat != 0) {
    fprintf(stderr, "am: failed to create supervisor thread\n");
    return 1;
  }

  while (1) {
    pause(); 
  }

  // unreachable — kept for completeness
  free(app_info_list.app);
  rbuflogd_logger_close();
  return 0;
}


static app_config_list_t * filter_app_info_by_phase(
    const char * phase, const app_config_list_t * app_config_list) {

  app_config_list_t * filtered_list = (app_config_list_t *)malloc(sizeof(app_config_list_t));
  int num_of_apps_in_phase = 0;
  for (int i = 0; i < app_config_list->num_apps; i++) {
    app_config_t * app_cfg = &app_config_list->app[i];
    for (int j = 0; j < app_cfg->num_phases; j++) {
      if (strcmp(app_cfg->phases[j], phase) == 0) {
        num_of_apps_in_phase++;
        break;
      }
    }
  }
  filtered_list->app = (app_config_t *)malloc(num_of_apps_in_phase * sizeof(app_config_t));
  filtered_list->num_apps = num_of_apps_in_phase;
  int index = 0;
  for (int i = 0; i < app_config_list->num_apps; i++) {
    app_config_t * app_cfg = &app_config_list->app[i];
    for (int j = 0; j < app_cfg->num_phases; j++) {
      if (strcmp(app_cfg->phases[j], phase) == 0) {
        filtered_list->app[index] = app_config_list->app[i];
        index++;
        break;
      }
    }
  }
  return filtered_list;

}


static void init_app_config_struct(
    app_config_list_t * app_config_list, const app_info_list_t * app_info_list) {
  
  for (int i = 0; i < app_config_list->num_apps; i++) {
    app_info_list->app[i].name = strdup(app_config_list->app[i].name);
    app_config_list->app[i].info = &app_info_list->app[i];
    app_info_list->app[i].status = APP_STATUS_NOT_STARTED;
  }
}


static int create_supervisor_thread(
    app_config_list_t * app_config_list, app_info_list_t * app_info_list) {
  // --- block SIGCHLD before forking any children so the supervisor thread
  //     is the sole recipient via sigwaitinfo — must happen before fork calls
  sigset_t sigchld_mask;
  sigemptyset(&sigchld_mask);
  sigaddset(&sigchld_mask, SIGCHLD);
  if (pthread_sigmask(SIG_BLOCK, &sigchld_mask, NULL) != 0) {
    log_error("main", "failed to block SIGCHLD");
    return 1;
  }

  supervisor_args_t * sup_args = malloc(sizeof(supervisor_args_t));
  if (sup_args == NULL) {
    log_error("main", "failed to allocate supervisor args");
    return 1;
  }
  sup_args->app_config_list = app_config_list;
  sup_args->app_info_list   = app_info_list;

  pthread_t sup_thread;
  const int thread_status = pthread_create(
    &sup_thread, 
    NULL, 
    supervisor_thread, 
    sup_args);

  if (thread_status != 0) {
    log_error("main", "failed to create supervisor thread");
    free(sup_args);
    return 1;
  }
  else {
    log_info("main", "supervisor thread started");
  }

  return 0; 
}

