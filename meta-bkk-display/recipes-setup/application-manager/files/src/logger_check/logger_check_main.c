#include "rbuflogd/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


static const int DEFAULT_TIMEOUT_SEC = 10;


int parse_cli(int argc, char ** argv, int * timeout_out_sec) {
  *timeout_out_sec = -1;
  for (int i = 1; i < argc; i++) {
    const char * arg = argv[i];
    if (strcmp(arg, "--timeout") == 0) {
      if (i + 1 >= argc) {
        *timeout_out_sec = DEFAULT_TIMEOUT_SEC;
        return -1;
      }
      i++;
      *timeout_out_sec = atoi(argv[i]);
    }
  }
  if (*timeout_out_sec == -1) {
    *timeout_out_sec = DEFAULT_TIMEOUT_SEC;
  }
  return 0;
}


int main(int argc, char * argv[]) {
  int timeout_sec;
  (void)parse_cli(argc, argv, &timeout_sec);
  
  // get start time: 
  struct timespec start_time, act_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);

  // start tring to connect to the logger: 
  while(1) {
    const int res = rbuflogd_logger_init("test"); 
    if (res == 0) {
      break;
    }

    clock_gettime(CLOCK_MONOTONIC, &act_time);
    const int elapsed_sec = act_time.tv_sec - start_time.tv_sec;
    if (elapsed_sec >= timeout_sec) {
      return -1; 
    }

    usleep(1000); 
  }

  return 0;
}