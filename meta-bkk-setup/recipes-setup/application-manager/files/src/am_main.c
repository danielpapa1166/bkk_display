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
static const int LOGGER_APP_INDEX = 0;
static void start_logger_process(boot_mode_t boot_mode,
  app_config_t * logger_cfg, app_info_list_t * app_info_list); 
static void log_app_launch_status(
  int app_index, app_config_t * app_cfg, launch_status_t status); 


// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------

int main(int argc, char * argv[])
{
  // --- parse CLI ---
  char * config_path = NULL;
  parse_status_t status = parse_cli(argc, argv, &config_path);
  if (status != PARSE_OK) {
    fprintf(stderr, "am: invalid command line arguments\n");
    return 1;
  }

  // --- parse config file ---
  app_config_list_t config_list;
  status = parse_config(config_path, &config_list);
  if (status != PARSE_OK) {
    fprintf(stderr, "am: failed to parse config file\n");
    return 1;
  }

  const boot_mode_t boot_mode = determine_boot_mode();

  app_info_list_t app_info_list = {
    .app = (app_info_t *)calloc(
      config_list.num_apps, sizeof(app_info_t)),
    .num_apps = config_list.num_apps
  };

  if (app_info_list.app == NULL) {
    fprintf(stderr, "am: failed to allocate app_info array\n");
    return 1;
  }

  // --- start logger first --- 
  config_list.apps[LOGGER_APP_INDEX].info = &app_info_list.app[LOGGER_APP_INDEX];
  start_logger_process(
    boot_mode,
    &config_list.apps[LOGGER_APP_INDEX], 
    &app_info_list);
  usleep(1000000); 

  // --- open logger after launching rbuflogd ---
  if (rbuflogd_logger_init("   AM   ") != 0) {
    fprintf(stderr, 
      "am: rbuflogd not yet available, continuing with stderr only\n");
  }

  char log_buf[256];
  if(boot_mode != BOOT_MODE_UNDEFINED) {
    snprintf(log_buf, sizeof(log_buf), 
      "Starting application in boot mode %s", 
      boot_mode_to_string(boot_mode));
    log_info("main", log_buf);
  }
  else {
    log_error("main", "Failed to determine boot mode");
  }

  // --- block SIGCHLD before forking any children so the supervisor thread
  //     is the sole recipient via sigwaitinfo — must happen before fork calls
  sigset_t sigchld_mask;
  sigemptyset(&sigchld_mask);
  sigaddset(&sigchld_mask, SIGCHLD);
  if (pthread_sigmask(SIG_BLOCK, &sigchld_mask, NULL) != 0) {
    log_error("main", "failed to block SIGCHLD");
    return 1;
  }

  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  supervisor_args_t sup_args = {
    .app_infos = &app_info_list,
    .lock = &lock
  };
  pthread_t sup_thread;
  const int thread_status = pthread_create(
    &sup_thread, 
    NULL, 
    supervisor_thread, 
    &sup_args);

  if (thread_status != 0) {
    log_error("main", "failed to create supervisor thread");
    return 1;
  }
  else {
    log_info("main", "supervisor thread started");
  }

  for (int i = LOGGER_APP_INDEX + 1; i < config_list.num_apps; i++) {
    // reference to app info struct for monitoring 
    config_list.apps[i].info = &app_info_list.app[i];
    app_info_list.app[i].status = APP_STATUS_NOT_STARTED;

    const launch_status_t status = launch_app(
      boot_mode,
      &config_list.apps[i], 
      &app_info_list);

    log_app_launch_status(i, &config_list.apps[i], status);
  }

  int cnt = 0; 
  while (1) {

    const int app_idx = cnt % config_list.num_apps;
    const launch_status_t status = launch_app(
      boot_mode,
      &config_list.apps[app_idx], 
      &app_info_list);

    log_app_launch_status(
      app_idx, 
      &config_list.apps[app_idx], status);

    usleep(1000000);
    cnt++;
  }

  // unreachable — kept for completeness
  free(app_info_list.app);
  rbuflogd_logger_close();
  return 0;
}



static void start_logger_process(boot_mode_t boot_mode,
    app_config_t * logger_cfg, app_info_list_t * app_info) {
    
  const launch_status_t status = launch_app(boot_mode, 
    logger_cfg, app_info);
  if (status != LAUNCH_OK) {
    fprintf(stderr, "am: failed to launch logger '%s'\n", logger_cfg->name);
  }
  else {
    fprintf(stderr, "am: launched logger '%s' successfully\n", logger_cfg->name);
  }
}


static void log_app_launch_status(
    int app_index, app_config_t * app_cfg, launch_status_t status) {

  char log_buf[256];
  if (status == LAUNCH_OK) {
    snprintf(log_buf, sizeof(log_buf), 
      "Launched app #%d '%s' successfully", 
      app_index, 
      app_cfg->name);

    log_info("main", log_buf);
  } 
  else if(status == LAUNCH_OK_NOT_LAUNCHED) {
    (void) status; 
    // do nothing 
  }
  else if(status == LAUNCH_OK_DELAYED_LAUNCH) {
    snprintf(log_buf, sizeof(log_buf), 
      "App #%d '%s' prerequisites not met, will attempt delayed launch", 
      app_index, 
      app_cfg->name);

    log_info("main", log_buf);
  }
  else {
    snprintf(log_buf, sizeof(log_buf), 
      "Failed to launch app #%d'%s': %s", 
      app_index,
      app_cfg->name, 
      launch_status_to_string(status));

    log_error("main", log_buf);

  }
}