#include <stdio.h>
#include <stdlib.h>

#include <cJSON.h>
#include <rbuflogd/producer.h>

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

  // --- cJSON smoke test --- 
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "status", "ok");
  cJSON_AddStringToObject(root, "app", "application_manager");

  char *json_str = cJSON_Print(root);
  printf("application_manager: %s\n", json_str);
  free(json_str);
  cJSON_Delete(root);

  return 0;
}
