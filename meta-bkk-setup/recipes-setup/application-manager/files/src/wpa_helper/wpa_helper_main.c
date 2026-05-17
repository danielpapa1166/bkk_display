#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include "wpa_config.h"
#include "rbuflogd/logger.h"

// ----------------------------------------------------------------------------
// local types 
// ----------------------------------------------------------------------------

typedef enum {
  BOOT_MODE_WIFI_CONFIG,
  BOOT_MODE_API_CONFIG,
  BOOT_MODE_NORMAL,
  BOOT_MODE_UNKNOWN
} boot_mode_t;

// ----------------------------------------------------------------------------
// local function prototypes
// ----------------------------------------------------------------------------

static boot_mode_t parse_args(int argc, char *argv[], wpa_config_t *config);
static int match_key(const char *arg, const char *key, const char **value);
static int prepare_config_folder(const char *path);
static int write_wpa_config(
  const char *path, const char *name, const char *content);


// ----------------------------------------------------------------------------
// main entry point
// ----------------------------------------------------------------------------
char msg_buf[256];

int main(int argc, char *argv[])
{
  rbuflogd_logger_init("wpa_help");
  wpa_config_t config = {0};
  const boot_mode_t boot_mode = parse_args(argc, argv, &config);

  
  if(boot_mode == BOOT_MODE_WIFI_CONFIG) {
    log_info("main", "Writing WPA config in WIFI_CONFIG mode");
  }
  else if(boot_mode == BOOT_MODE_API_CONFIG) {
    log_info("main", "Writing WPA config in API_CONFIG mode");
  }
  else if(boot_mode == BOOT_MODE_NORMAL) {
    log_info("main", "Writing WPA config in NORMAL mode");
  }
  else {
    log_error("main", "Invalid or missing boot_mode argument");
    rbuflogd_logger_close(); 
    return -1;
  }
  
  int res = prepare_config_folder(config.wpa_cfg_path);
  if (res != 0) {
    log_error("main", "Fatal. "
      "Failed to prepare WPA config folder. App exiting.");
    rbuflogd_logger_close();
    return -1;
  }

  res = prepare_config_folder(config.network_cfg_path);
  if (res != 0) {
    log_error("main", "Fatal. "
      "Failed to prepare network config folder. App exiting.");
    rbuflogd_logger_close();
    return -1;
  }

  res = write_wpa_config(
    config.wpa_cfg_path, 
    config.wpa_cfg_name, 
    config.wpa_cfg_str);
  if (res != 0) {
    log_error("main", "Fatal. "
      "Failed to write WPA config. App exiting.");
    rbuflogd_logger_close();
    return -1;
  }

  res = write_wpa_config(
    config.network_cfg_path, 
    config.network_cfg_name, 
    config.network_cfg_str);
  if (res != 0) {
    log_error("main", "Fatal. "
      "Failed to write network config. App exiting.");
    rbuflogd_logger_close();
    return -1;
  }

  log_info("main", "WPA config written successfully. App exiting.");

  rbuflogd_logger_close();
  return 0;
}

// ----------------------------------------------------------------------------
// local function implementations
// ----------------------------------------------------------------------------

/* Returns 1 and sets *value to the argument's value portion if the argument
   matches "key=<value>", otherwise returns 0. */
static int match_key(const char *arg, const char *key, const char **value) {
  const size_t key_len = strlen(key);
  if (strncmp(arg, key, key_len) == 0 && arg[key_len] == '=') {
    *value = (const char *)(arg + key_len + 1);
    return 1;
  }
  return 0;
}


static boot_mode_t parse_args(int argc, char *argv[], wpa_config_t *config) {
  boot_mode_t boot_mode = BOOT_MODE_UNKNOWN;

  if (argc < 2) {
    return boot_mode;
  }

  /* First argument is mandatory: boot_mode=<value> */
  char *mode_val = NULL;
  if (!match_key(argv[1], "boot_mode", 
      (const char **)&mode_val)) {
    return boot_mode;
  }

  if (strcmp(mode_val, "wifi_config") == 0) {
    boot_mode = BOOT_MODE_WIFI_CONFIG;
  } 
  else if (strcmp(mode_val, "api_config") == 0) {
    boot_mode = BOOT_MODE_API_CONFIG;
  } 
  else if (strcmp(mode_val, "normal") == 0) {
    boot_mode = BOOT_MODE_NORMAL;
  } 
  else {
    return boot_mode;
  }

  // assign default config:
  if(boot_mode == BOOT_MODE_WIFI_CONFIG) {
    *config = wpa_ap_config;
  }
  else if(boot_mode == BOOT_MODE_API_CONFIG || boot_mode == BOOT_MODE_NORMAL) {
    *config = wpa_wifi_config;
  }
  else {
    return boot_mode;
  }

  // Remaining arguments are optional key=value pairs 
  for (int i = 2; i < argc; i++) {
    const char *val = NULL;
    if      (match_key(argv[i], "wpa_cfg_path",     &val)) { 
      config->wpa_cfg_path     = val; 
    }
    else if (match_key(argv[i], "network_cfg_path",  &val)) { 
      config->network_cfg_path  = val; 
    }
    else if (match_key(argv[i], "wpa_cfg_name",      &val)) { 
      config->wpa_cfg_name      = val; 
    }
    else if (match_key(argv[i], "network_cfg_name",  &val)) { 
      config->network_cfg_name  = val; 
    }
  }

  return boot_mode;
}


static int prepare_config_folder(const char *path) {
  if (path == NULL) {
    return 0;
  }

  // todo: log error 


  struct stat st;
  if (stat(path, &st) == 0) {
    if (S_ISDIR(st.st_mode)) {
      return 0;
    }
    snprintf(
      msg_buf, 
      sizeof(msg_buf), 
      "prepare_config_folder: path exists but is not a directory: %s", path);
    log_error("wr cfg", msg_buf);
    return -1;
  }

  if (errno != ENOENT) {
    snprintf(
      msg_buf, 
      sizeof(msg_buf), 
      "prepare_config_folder: stat failed for %s: %s", 
      path, strerror(errno));
    log_error("wr cfg", msg_buf);
    return -1;
  }

  if (mkdir(path, 0755) != 0 && errno != EEXIST) {
    snprintf(
      msg_buf, 
      sizeof(msg_buf), 
      "prepare_config_folder: failed to create directory %s: %s", 
      path, strerror(errno));
    log_error("wr cfg", msg_buf);
    return -1;
  }

  return 0;
}


static int write_wpa_config(const char *path, const char *name, const char *content) {
  if (path == NULL || name == NULL || content == NULL) {
    fprintf(stderr, "write_file: NULL argument\n");
    return -1;
  }

  char full_path[PATH_MAX];
  int n = snprintf(
    full_path, 
    sizeof(full_path), 
    "%s/%s", path, name);

  if (n < 0 || (size_t)n >= sizeof(full_path)) {
    snprintf(
      msg_buf, 
      sizeof(msg_buf), 
      "write_file: path too long: %s/%s", 
      path, name);
    log_error("wr cfg", msg_buf);
    return -1;
  }

  FILE *f = fopen(full_path, "w");
  if (f == NULL) {
    snprintf(
      msg_buf, 
      sizeof(msg_buf), 
      "write_file: failed to open %s: %s", 
      full_path, strerror(errno));
    log_error("wr cfg", msg_buf);
    return -1;
  }

  const size_t content_len = strlen(content);
  const size_t written = fwrite(content, 1, content_len, f);
  fclose(f);

  if (written != content_len) {
    snprintf(
      msg_buf, 
      sizeof(msg_buf), 
      "write_file: incomplete write to %s. Written %zu of %zu bytes", 
      full_path, written, content_len);
    log_error("wr cfg", msg_buf);
    return -1;
  }

  return 0;
}