#include "rbuflogd/logger.h"
#include "am_supervisor.h"
#include "am_launcher.h"
#include "am_types.h"
#include "am_boot_mode.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>



static void log_app_launch_status(
    int app_index, app_config_t * app_cfg, launch_status_t status) {

  const char * log_cat = "launcher"; 
  char log_buf[256];
  if (status == LAUNCH_OK) {
    snprintf(log_buf, sizeof(log_buf), 
      "Launched app #%d '%s' successfully", 
      app_index, 
      app_cfg->name);

    log_info(log_cat, log_buf);
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

    log_info(log_cat, log_buf);
  }
  else {
    snprintf(log_buf, sizeof(log_buf), 
      "Failed to launch app #%d'%s': %s", 
      app_index,
      app_cfg->name, 
      launch_status_to_string(status));

    log_error(log_cat, log_buf);

  }
}


typedef enum {
  TRIGGER_NO_NEW_LAUNCH,
  TRIGGER_LAUNCHED_NEW_APP
} trigger_status_t;

static trigger_status_t trigger_app_start(app_config_list_t * app_cfg_list, app_info_list_t * app_info_list) {
  trigger_status_t result = TRIGGER_NO_NEW_LAUNCH;
  for (int i = 0; i < app_cfg_list->num_apps; i++) {

    const launch_status_t status = launch_app(
      &app_cfg_list->app[i], 
      app_info_list);

    log_app_launch_status(i, &app_cfg_list->app[i], status);

    if (status == LAUNCH_OK) {
      result = TRIGGER_LAUNCHED_NEW_APP;
    }
  }
  return result;
}


static void init_logger(void) {
  if (rbuflogd_logger_init("   AM   ") != 0) {
    fprintf(stderr,
      "am: rbuflogd not yet available, continuing with stderr only\n");
  }

  log_info("main", "Application Manager starting up");
  const boot_mode_t boot_mode = get_boot_mode();
  char log_buf[256];
  if (boot_mode != BOOT_MODE_UNDEFINED) {
    snprintf(log_buf, sizeof(log_buf),
      "Starting application in boot mode %s",
      boot_mode_to_string(boot_mode));
    log_info("main", log_buf);
  }
  else {
    log_error("main", "Failed to determine boot mode");
  }
}


// Reap all children that have exited, update app_info status
static void reap_children(app_info_list_t * app_info_list, const siginfo_t * siginfo) {
  pid_t pid;
  int wstatus;

  const char * log_cat = "sprvsr";
  const int num_apps = app_info_list->num_apps; 
  app_info_t * app_infos = app_info_list->app;


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
        log_info(log_cat, msg);
        if (app_infos[i].exit_code == 0 &&
            app_infos[i].name != NULL &&
            strcmp(app_infos[i].name, "logger_check") == 0) {
          init_logger();
        }
      } 
      else if (WIFSIGNALED(wstatus)) {
        app_infos[i].status = APP_STATUS_KILLED;
        app_infos[i].exit_code = -1; // Indicate killed by signal
        int sig = WTERMSIG(wstatus);
        char msg[256];
        snprintf(msg, sizeof(msg), "'%s' (pid %d) killed by signal %d (%s)%s",
                 app_infos[i].name, pid, sig, strsignal(sig),
                 WCOREDUMP(wstatus) ? " [core dumped]" : "");
        log_info(log_cat, msg);
        // If signal came from outside the process, log the sender
        if (siginfo && siginfo->si_pid != 0 && siginfo->si_pid != pid) {
          char sender[128];
          snprintf(sender, sizeof(sender),
                   "'%s' signal sender: pid %d uid %d (si_code=%d)",
                   app_infos[i].name, siginfo->si_pid, siginfo->si_uid, siginfo->si_code);
          log_info(log_cat, sender);
        }
      }
      break;
    }
  }
}


void * supervisor_thread(void * args) {

  supervisor_args_t * sup_args          = (supervisor_args_t *)args;
  app_config_list_t * app_config_list   = sup_args->app_config_list;
  app_info_list_t   * app_info_list     = sup_args->app_info_list;
  free(sup_args);

  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  siginfo_t info;

  while (trigger_app_start(app_config_list, app_info_list));

  while (1) {
    if (sigwaitinfo(&mask, &info) > 0) {

      reap_children(app_info_list, &info);

      while (trigger_app_start(app_config_list, app_info_list));
    }
  }
  return NULL;

}