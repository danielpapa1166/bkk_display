#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <systemd/sd-bus.h>
#include "rbuflogd/logger.h"

/* D-Bus address of the systemd-networkd.service unit.
 * The unit name is escaped per systemd D-Bus conventions:
 *   '-'  ->  '_2d'
 *   '.'  ->  '_2e'  */
#define SYSTEMD_BUS_NAME  "org.freedesktop.systemd1"
#define NETWORKD_OBJ_PATH \
  "/org/freedesktop/systemd1/unit/systemd_2dnetworkd_2eservice"
#define UNIT_IFACE    "org.freedesktop.systemd1.Unit"

#define DEFAULT_TIMEOUT_S  30
#define POLL_INTERVAL_US   500000  /* 500 ms */

char msg_buf[256];

/* ----------------------------------------------------------------------------
 * Returns 1 if systemd-networkd ActiveState == "active", 0 otherwise.
 * D-Bus errors are logged as warnings so the caller keeps retrying.
 * -------------------------------------------------------------------------- */
static int query_active_state(sd_bus *bus)
{
  sd_bus_error error = SD_BUS_ERROR_NULL;
  char *state = NULL;

  const int r = sd_bus_get_property_string(
    bus,
    SYSTEMD_BUS_NAME,
    NETWORKD_OBJ_PATH,
    UNIT_IFACE,
    "ActiveState",
    &error,
    &state);

  if (r < 0) {
    snprintf(msg_buf, sizeof(msg_buf),
      "ActiveState query failed: %s",
      error.message ? error.message : strerror(-r));
    log_warning("netd_chk", msg_buf);
    sd_bus_error_free(&error);
    return 0;
  }

  const int active = (strcmp(state, "active") == 0);
  free(state);
  sd_bus_error_free(&error);
  return active;
}

/* ----------------------------------------------------------------------------
 * Parse --timeout <seconds> from argv; return DEFAULT_TIMEOUT_S if absent.
 * -------------------------------------------------------------------------- */
static int parse_timeout(int argc, char *argv[])
{
  for (int i = 1; i < argc - 1; i++) {
    if (strcmp(argv[i], "--timeout") == 0) {
      const int t = atoi(argv[i + 1]);
      return (t > 0) ? t : DEFAULT_TIMEOUT_S;
    }
  }
  return DEFAULT_TIMEOUT_S;
}

/* ----------------------------------------------------------------------------
 * main
 *
 * Exit codes:
 *   0  – networkd became active and networkctl reload succeeded
 *   1  – timed out waiting for networkd
 *   2  – fatal error (D-Bus unavailable, execv failed, …)
 *
 * The application manager treats any non-zero exit as PREREQ_FAILED, which
 * blocks dependent apps (wpa_supplicant) from launching.
 * -------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
  rbuflogd_logger_init("netd_chk");

  const int timeout_s = parse_timeout(argc, argv);

  snprintf(msg_buf, sizeof(msg_buf),
    "Waiting for systemd-networkd (timeout %ds)", timeout_s);
  log_info("main", msg_buf);

  struct timespec start_time, act_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  int active = 0;
  sd_bus *bus = NULL;

  clock_gettime(CLOCK_MONOTONIC, &act_time);
  while ((act_time.tv_sec - start_time.tv_sec) < timeout_s) {
    if (bus == NULL) {
      if (sd_bus_open_system(&bus) < 0) {
        /* D-Bus not up yet; keep waiting */
        bus = NULL;
        usleep(POLL_INTERVAL_US);
        clock_gettime(CLOCK_MONOTONIC, &act_time);
        continue;
      }
      /* Cap each D-Bus call to 1 s so the wall-clock deadline governs
       * the loop, not the sd-bus internal default (~25 s). */
      sd_bus_set_method_call_timeout(bus, 1000000 /* 1 s in µs */);
    }

    if (query_active_state(bus)) {
      active = 1;
      break;
    }
    usleep(POLL_INTERVAL_US);
    clock_gettime(CLOCK_MONOTONIC, &act_time);
  }

  if (bus == NULL) {
    log_error("main", "Failed to connect to system D-Bus within timeout");
    rbuflogd_logger_close();
    return 2;
  }

  sd_bus_unref(bus);

  if (!active) {
    log_error("main", "Timed out waiting for systemd-networkd");
    rbuflogd_logger_close();
    return 1;
  }

  log_info("main", "systemd-networkd active, reloading network config");
  rbuflogd_logger_close();

  /* Replace this process with networkctl reload so its exit code is
   * returned directly to the application manager's prerequisite check. */
  char *args[] = { "/bin/networkctl", "reload", NULL };
  execv(args[0], args);

  /* execv only returns on failure */
  perror("execv networkctl");
  return 2;
}
