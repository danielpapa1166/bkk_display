#include "rbuflogd/logger.h"
#include "am_supervisor.h"
#include "am_types.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t g_child_exited = 0;

static void sigchld_handler(int sig) {
  (void)sig;
  g_child_exited = 1;
}


// Reap all children that have exited, update app_info status
static void reap_children(app_info_t * app_infos, int num_apps, const siginfo_t * siginfo) {
  pid_t pid;
  int wstatus;

  while ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
    for (int i = 0; i < num_apps; i++) {
      if (app_infos[i].pid != pid) {
        continue;
      }
      if (WIFEXITED(wstatus)) {
        app_infos[i].status = APP_STATUS_EXITED;
        app_infos[i].exit_code = WEXITSTATUS(wstatus);
        char msg[128];
        snprintf(msg, sizeof(msg), "'%s' (pid %d) exited with code %d",
                 app_infos[i].name, pid, app_infos[i].exit_code);
        log_info("supervisor", msg);
      } 
      else if (WIFSIGNALED(wstatus)) {
        app_infos[i].status = APP_STATUS_KILLED;
        app_infos[i].exit_code = -1; // Indicate killed by signal
        int sig = WTERMSIG(wstatus);
        char msg[256];
        snprintf(msg, sizeof(msg), "'%s' (pid %d) killed by signal %d (%s)%s",
                 app_infos[i].name, pid, sig, strsignal(sig),
                 WCOREDUMP(wstatus) ? " [core dumped]" : "");
        log_info("supervisor", msg);
        // If signal came from outside the process, log the sender
        if (siginfo && siginfo->si_pid != 0 && siginfo->si_pid != pid) {
          char sender[128];
          snprintf(sender, sizeof(sender),
                   "'%s' signal sender: pid %d uid %d (si_code=%d)",
                   app_infos[i].name, siginfo->si_pid, siginfo->si_uid, siginfo->si_code);
          log_info("supervisor", sender);
        }
      }
      break;
    }
  }
}


void * supervisor_thread(void * args) {
  
  supervisor_args_t * sup_args = (supervisor_args_t *)args;
  app_info_t * app_infos = sup_args->app_infos;
  int num_apps = sup_args->num_apps;
  pthread_mutex_t * lock = sup_args->lock;
  
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  siginfo_t info;
        
  while (1) {
    if (sigwaitinfo(&mask, &info) > 0) {
      pthread_mutex_lock(lock);
      reap_children(app_infos, num_apps, &info);
      pthread_mutex_unlock(lock);
    }
  }
  return NULL;

}