#include "supplicant_handler.h"
#include <signal.h>
#include <rbuflogd/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PID_COUNT 64
#define MAX_PID_LINE_LENGTH 1024

static pid_t wpa_supplicant_child_pid = -1;


// kill every wpa_supplicant process running on the system. 
// This is a brute-force approach to ensure 
// that no wpa_supplicant processes are left running, 
// which could interfere with the network manager's operation.
void kill_all_supplicant_processes() { // todo: review this 
  char pidline[MAX_PID_LINE_LENGTH];
  char *pid;
  int i = 0;
  int pidno[MAX_PID_COUNT];
  FILE *fp = popen("pidof wpa_supplicant","r");

  if(fp == NULL) {
    log_error("Supp", "Failed to execute pidof command.");
    return;
  }

  fgets(pidline, MAX_PID_LINE_LENGTH, fp);

  printf("pid line: %s\n", pidline);
  pid = strtok(pidline," ");
  if(pid == NULL) {
    printf("No wpa_supplicant processes found.\n");
    log_info("Supp", "No wpa_supplicant processes found.");
    pclose(fp);
    return;
  }

  printf("pid: %s\n", pid);
  while(pid != NULL) {
    pidno[i] = atoi(pid);

    if(pidno[i] <= 0) {
      log_warning("Supp", "Invalid PID found, skipping.");
      pid = strtok(NULL , " ");
      continue;
    }

    printf("killing %d\n",pidno[i]);
    kill(pidno[i], SIGKILL);
    pid = strtok(NULL , " ");
    i++;

    if(i >= MAX_PID_COUNT) {
      log_warning("Supp", 
        "Reached maximum PID count, some processes may not be killed.");  
      break;
    }
  }

  pclose(fp);

  printf("All wpa_supplicant processes killed.\n");
}


int start_supplicant(
  char * const wpa_cfg_path, 
  char * const wpa_interface_name) {
  char msg_buf[256];
  snprintf(msg_buf, sizeof(msg_buf),
    "Starting wpa_supplicant with config: %s", wpa_cfg_path);
  log_info("Supp", msg_buf);

  pid_t pid = fork();
  if (pid < 0) {
    log_error("Supp", "Failed to fork for wpa_supplicant.");
    return -1;
  } 
  else if (pid == 0) {
    // Child process
    char * const args[] = {
      "wpa_supplicant",
      "-B", // run in background
      "-i", wpa_interface_name,
      "-c", wpa_cfg_path,
      NULL
    };
    execv(
      "/usr/sbin/wpa_supplicant",
      args
    );
    // If execlp returns, there was an error
    log_error("Supp", "Failed to exec wpa_supplicant.");
    exit(EXIT_FAILURE);
  }

  // Parent process
  log_info("Supp", "wpa_supplicant started successfully.");
  wpa_supplicant_child_pid = pid;
  return 0;
}


int stop_supplicant() {
  if (wpa_supplicant_child_pid > 0) {
    log_info("Supp", "Stopping wpa_supplicant.");
    kill(wpa_supplicant_child_pid, SIGTERM);
    wpa_supplicant_child_pid = -1;
    return 0;
  } 
  else {
    log_info("Supp", "No wpa_supplicant process to stop.");
    return -1;
  }
}

