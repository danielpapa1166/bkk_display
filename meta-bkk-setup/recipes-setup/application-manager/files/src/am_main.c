#include <stdio.h>
#include <stdlib.h>


#include <rbuflogd/producer.h>
#include "am_config_parser.h"


// add argument parser: config JSON added as cli arg 
// aplication config struct (mirroring the config JSON)
// load at startup 

// define config struct !!!!

int main(void)
{
  // --- rbuflogd smoke test --- 
  rbuflogd_producer_t log;
  if (rbuflogd_producer_open(&log, "am") == 0) {
    rbuflogd_producer_log(&log, RBUF_LOG_LEVEL_INFO, "init",
      "application_manager starting");
    rbuflogd_producer_close(&log);
  } else {
    fprintf(stderr, "application_manager: rbuflogd not available, "
      "continuing without structured logging\n");
  }

  return 0;
}
