#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "wpa_config.h"

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
static int ensure_dir(const char *path); 
static int match_key(const char *arg, const char *key, const char **value);
static int prepare_config_folder(const wpa_config_t * const config);
static int write_wpa_config(const wpa_config_t * const config);


// ----------------------------------------------------------------------------
// main entry point
// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  wpa_config_t config = wpa_ap_config;
  const boot_mode_t boot_mode = parse_args(argc, argv, &config);

  if(boot_mode == BOOT_MODE_UNKNOWN) {
    // todo log here
    return -1;
  }

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
  if (!match_key(argv[1], "boot_mode", (const char **)&mode_val)) {
    return boot_mode;
  }

  if (strcmp(mode_val, "wifi_config") == 0) {
    boot_mode = BOOT_MODE_WIFI_CONFIG;
  } else if (strcmp(mode_val, "api_config") == 0) {
    boot_mode = BOOT_MODE_API_CONFIG;
  } else if (strcmp(mode_val, "normal") == 0) {
    boot_mode = BOOT_MODE_NORMAL;
  } else {
    return boot_mode;
  }

  /* Remaining arguments are optional key=value pairs */
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


static int ensure_dir(const char *path) {
  if (path == NULL) {
    return 0;
  }

  // todo: log error 


  struct stat st;
  if (stat(path, &st) == 0) {
    if (S_ISDIR(st.st_mode)) {
      return 0;
    }
    fprintf(stderr, "prepare_config_folder: path exists but is not a directory: %s\n", path);
    return -1;
  }

  if (errno != ENOENT) {
    fprintf(stderr, "prepare_config_folder: stat failed for %s: %m\n", path);
    return -1;
  }

  if (mkdir(path, 0755) != 0 && errno != EEXIST) {
    fprintf(stderr, "prepare_config_folder: failed to create directory %s: %m\n", path);
    return -1;
  }

  return 0;
}


static int prepare_config_folder(const wpa_config_t * const config) {
  if (ensure_dir(config->wpa_cfg_path) != 0) {
    return -1;
  }
  if (ensure_dir(config->network_cfg_path) != 0) {
    return -1;
  }
  return 0;
}


static int write_wpa_config(const wpa_config_t * const config) {

}