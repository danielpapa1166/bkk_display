#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <rbuflogd/producer.h>
#include "am_config_parser.h"
#include "am_launcher.h"
#include "am_types.h"

// ----------------------------------------------------------------------------
// Globals used by the SIGCHLD handler
// ----------------------------------------------------------------------------

static volatile sig_atomic_t g_child_exited = 0;

static void sigchld_handler(int sig) {
  (void)sig;
  g_child_exited = 1;
}

// ----------------------------------------------------------------------------
// Reap all children that have exited, update app_info status
// ----------------------------------------------------------------------------

static void reap_children(app_info_t * app_infos, int num_apps,
                           rbuflogd_producer_t * log) {
  pid_t pid;
  int wstatus;

  while ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
    for (int i = 0; i < num_apps; i++) {
      if (app_infos[i].pid != pid) {
        continue;
      }
      if (WIFEXITED(wstatus)) {
        app_infos[i].status = APP_STATUS_EXITED;
        char msg[128];
        snprintf(msg, sizeof(msg), "'%s' (pid %d) exited with code %d",
                 app_infos[i].name, pid, WEXITSTATUS(wstatus));
        fprintf(stderr, "am: %s\n", msg);
        rbuflogd_producer_log(log, RBUF_LOG_LEVEL_WARNING, "lifecycle", msg);
      } else if (WIFSIGNALED(wstatus)) {
        app_infos[i].status = APP_STATUS_EXITED;
        char msg[128];
        snprintf(msg, sizeof(msg), "'%s' (pid %d) killed by signal %d",
                 app_infos[i].name, pid, WTERMSIG(wstatus));
        fprintf(stderr, "am: %s\n", msg);
        rbuflogd_producer_log(log, RBUF_LOG_LEVEL_ERROR, "lifecycle", msg);
      }
      break;
    }
  }
}

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

  // --- install SIGCHLD handler before any fork ---
  struct sigaction sa = {0};
  sa.sa_handler = sigchld_handler;
  sa.sa_flags   = SA_RESTART;   // don't interrupt blocking syscalls
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGCHLD, &sa, NULL) != 0) {
    fprintf(stderr, "am: failed to install SIGCHLD handler\n");
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
  while (1) {
    pause(); // sleep until any signal arrives

    if (g_child_exited) {
      g_child_exited = 0;
      reap_children(app_infos, config_list.num_apps, &log);
    }
  }

  // unreachable — kept for completeness
  free(app_infos);
  rbuflogd_producer_close(&log);
  return 0;
}
