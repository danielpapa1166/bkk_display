#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <rbuflogd/producer.h>
#include "am_config_parser.h"
#include "am_launcher.h"
#include "am_types.h"
#include "am_supervisor.h"




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

  // --- open logger after launching apps (rbuflogd is a managed child) ---
  rbuflogd_producer_t log;
  if (rbuflogd_producer_open(&log, "am") != 0) {
    fprintf(stderr, "am: rbuflogd not yet available, continuing with stderr only\n");
  }
  rbuflogd_producer_log(&log, RBUF_LOG_LEVEL_INFO, "init",
                        "application_manager entering event loop");

  // --- main event loop: sleep until SIGCHLD wakes us, then reap ---
  start_supervisor(app_infos, config_list.num_apps);

  // unreachable — kept for completeness
  free(app_infos);
  rbuflogd_producer_close(&log);
  return 0;
}
