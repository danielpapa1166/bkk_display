#include "am_supervisor.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t g_child_exited = 0;

static void sigchld_handler(int sig) {
  (void)sig;
  g_child_exited = 1;
}


// Reap all children that have exited, update app_info status
static void reap_children(app_info_t * app_infos, int num_apps) {
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
        printf("am: %s\n", msg);
      } 
      else if (WIFSIGNALED(wstatus)) {
        app_infos[i].status = APP_STATUS_EXITED;
        char msg[128];
        snprintf(msg, sizeof(msg), "'%s' (pid %d) killed by signal %d",
                 app_infos[i].name, pid, WTERMSIG(wstatus));
        printf("am: %s\n", msg);
      }
      break;
    }
  }
}



int start_supervisor(app_info_t * app_infos, int num_apps) {

  // --- install SIGCHLD handler before any fork ---
  struct sigaction sa = {0};
  sa.sa_handler = sigchld_handler;
  sa.sa_flags   = SA_RESTART;   // don't interrupt blocking syscalls
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGCHLD, &sa, NULL) != 0) {
    fprintf(stderr, "am: failed to install SIGCHLD handler\n");
    return 1;
  }

  while (1) {
    if (g_child_exited) {
      reap_children(app_infos, num_apps);
      g_child_exited = 0;
    }
    sleep(1);
  }
  return 0;
}
