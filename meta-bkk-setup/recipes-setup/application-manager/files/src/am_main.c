#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

#include "am_config_parser.h"
#include "am_launcher.h"
#include "am_types.h"
#include "am_supervisor.h"
#include "am_logger.h"

// mkdir rbuflogd 

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


  // --- launch all apps ---
  app_info_t * app_infos = (app_info_t *)malloc(
      sizeof(app_info_t) * config_list.num_apps);
  if (app_infos == NULL) {
    fprintf(stderr, "am: failed to allocate app_info array\n");
    return 1;
  }

  for (int i = 0; i < config_list.num_apps; i++) {
    const int pid = launch_app(&config_list.apps[i], &app_infos[i]);
    if (pid < 0) {
      fprintf(stderr, "am: failed to launch '%s'\n", config_list.apps[i].name);
    } else {
      printf("am: launched '%s' with pid %d\n", config_list.apps[i].name, pid);
    }
  }

  usleep(1000000); 
  // --- open logger after launching apps (rbuflogd is a managed child) ---
  if (init_logger() != 0) {
    fprintf(stderr, "am: rbuflogd not yet available, continuing with stderr only\n");
  }
  log_info("init", "application_manager entering event loop");



  // block SIGCHLD before creating the supervisor thread so that sigwaitinfo
  // in the thread is the sole recipient — inherited by all threads spawned after
  sigset_t sigchld_mask;
  sigemptyset(&sigchld_mask);
  sigaddset(&sigchld_mask, SIGCHLD);
  if (pthread_sigmask(SIG_BLOCK, &sigchld_mask, NULL) != 0) {
    log_error("main", "failed to block SIGCHLD");
    return 1;
  }

  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  supervisor_args_t sup_args = {
    .app_infos = app_infos,
    .num_apps = config_list.num_apps,
    .lock = &lock
  };
  pthread_t sup_thread;
  if (pthread_create(&sup_thread, NULL, supervisor_thread, &sup_args) != 0) {
    log_error("main", "failed to create supervisor thread");
    return 1;
  }
  else {
    log_info("main", "supervisor thread started");
  }
  
  while (1) {
    pause();
  }

  // unreachable — kept for completeness
  free(app_infos);
  cleanup_logger();
  return 0;
}
