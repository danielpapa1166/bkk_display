#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include "cJSON.h"
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

typedef enum {
  WIFI_CRED_LOAD_SUCCESS,
  WIFI_CRED_LOAD_FALLBACK, 
  WIFI_CRED_LOAD_ERROR
} wifi_cred_load_status_t;

// ----------------------------------------------------------------------------
// local function prototypes
// ----------------------------------------------------------------------------

static boot_mode_t parse_args(int argc, char *argv[], wpa_config_t *config);
static int match_key(const char *arg, const char *key, const char **value);
static int prepare_config_folder(const char *path);
static int write_wpa_config(
  const char *path, const char *name, const char *content);
static void apply_fallback(char *ssid_out, size_t ssid_len,
  char *psk_out, size_t psk_len, const char *reason);
static wifi_cred_load_status_t load_wifi_credentials(
  const char *json_path,
  char *ssid_out, size_t ssid_len,
  char *psk_out,  size_t psk_len);
static int build_wpa_wifi_string(char *buf, size_t len,
  const char *ssid, const char *psk);


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
    log_info("main", "WIFI_CONFIG mode: writing wpa + network config");
  }
  else if(boot_mode == BOOT_MODE_API_CONFIG) {
    log_info("main", "API_CONFIG mode: writing network config only");
  }
  else if(boot_mode == BOOT_MODE_NORMAL) {
    log_info("main", "NORMAL mode: nothing to configure");
    rbuflogd_logger_close();
    return 0;
  }
  else {
    log_error("main", "Invalid or missing boot_mode argument");
    rbuflogd_logger_close(); 
    return -1;
  }

  /* Both WIFI_CONFIG and API_CONFIG need the network config folder/file */
  int res = prepare_config_folder(config.network_cfg_path);
  if (res != 0) {
    log_error("main", "Fatal. "
      "Failed to prepare network config folder. App exiting.");
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

  /* Only WIFI_CONFIG writes the wpa_supplicant config (AP credentials) */
  if (boot_mode == BOOT_MODE_WIFI_CONFIG) {
    res = prepare_config_folder(config.wpa_cfg_path);
    if (res != 0) {
      log_error("main", "Fatal. "
        "Failed to prepare WPA config folder. App exiting.");
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

  config->fallback_wifi_config_enable = 1; // default to enabled

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
    else if(match_key(argv[i], "fallback_wifi", &val)) {
      if(strcmp(val, "disable") == 0) {
        config->fallback_wifi_config_enable = 0;
      }
      else if(strcmp(val, "enable") == 0) {
        config->fallback_wifi_config_enable = 1;
      }
      else {
        char msg[100];
        snprintf(msg, sizeof(msg), 
          "Invalid value for fallback_wifi: %s. Expected 'enable' or 'disable'.", val);
        log_error("main", msg);
      }

      char msg[100];
      snprintf(msg, sizeof(msg), "Fallback WiFi config %s", 
        config->fallback_wifi_config_enable ? "enabled" : "disabled");
      log_debug("main", msg);
    }
  }

  return boot_mode;
}


static int prepare_config_folder(const char *path) {
  if (path == NULL) {
    log_error("wr cfg", "prepare_config_folder: NULL path argument");
    return 0;
  }

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


/* Copy the fallback credentials into the output buffers and log the reason. */
static void apply_fallback(
    char *ssid_out, size_t ssid_len,
    char *psk_out,  size_t psk_len,
    const char *reason)
{
  log_warning("wpa_cred", reason);
  strncpy(ssid_out, WPA_WIFI_FALLBACK_SSID, ssid_len - 1);
  ssid_out[ssid_len - 1] = '\0';
  strncpy(psk_out,  WPA_WIFI_FALLBACK_PSK,  psk_len  - 1);
  psk_out[psk_len - 1] = '\0';
}


/* Load SSID and password from WPA_WIFI_CREDENTIALS_JSON.
 * Copies the values into the caller-supplied buffers.
 * Returns 0 on success, 1 when the fallback (TeveClub) is used instead.
 * Never returns a negative value — a missing/corrupt file is not fatal;
 * the caller always gets a usable credential pair. */
static wifi_cred_load_status_t load_wifi_credentials(
    const char *json_path,
    char *ssid_out, size_t ssid_len,
    char *psk_out,  size_t psk_len)
{
  FILE *f = fopen(json_path, "r");
  if (f == NULL) {
    // json file should have been created by config-server in API_CONFIG mode, 
    // but if it's missing/unreadable for any reason that indicates 
    // a huger problem, log error and exit 
    snprintf(msg_buf, sizeof(msg_buf),
      "load_wifi_credentials: cannot open %s (%s) — using fallback",
      json_path, strerror(errno));
    
    log_error("wpa_cred", msg_buf); 
      
    return WIFI_CRED_LOAD_ERROR;
  }

  /* Read the whole file into a heap buffer */
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    apply_fallback(ssid_out, ssid_len, psk_out, psk_len,
      "load_wifi_credentials: fseek failed — using fallback");
    return WIFI_CRED_LOAD_FALLBACK;
  }

  const long file_size = ftell(f);
  if (file_size <= 0 || file_size > 4096) {
    fclose(f);
    apply_fallback(ssid_out, ssid_len, psk_out, psk_len,
      "load_wifi_credentials: file empty or too large — using fallback");
    return WIFI_CRED_LOAD_FALLBACK;
  }
  rewind(f);

  char *raw = (char *)malloc((size_t)file_size + 1);
  if (raw == NULL) {
    fclose(f);
    log_error("wpa_cred", "load_wifi_credentials: "
      "failed to allocate memory for file contents");
    return WIFI_CRED_LOAD_ERROR;
  }

  const size_t rd = fread(raw, 1, (size_t)file_size, f);
  fclose(f);
  raw[rd] = '\0';

  cJSON *root = cJSON_Parse(raw);
  free(raw);
  if (root == NULL) {
    apply_fallback(ssid_out, ssid_len, psk_out, psk_len,
      "load_wifi_credentials: JSON parse error — using fallback");
    return WIFI_CRED_LOAD_FALLBACK;
  }

  const cJSON *j_ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
  const cJSON *j_psk  = cJSON_GetObjectItemCaseSensitive(root, "password");

  if (!cJSON_IsString(j_ssid) || j_ssid->valuestring[0] == '\0' ||
      !cJSON_IsString(j_psk)  || j_psk->valuestring[0]  == '\0') {
    cJSON_Delete(root);
    apply_fallback(ssid_out, ssid_len, psk_out, psk_len,
      "load_wifi_credentials: missing/empty fields — using fallback");
    return WIFI_CRED_LOAD_FALLBACK;
  }

  /* Minimum SSID length guard (also catches test/placeholder values) */
  if (strlen(j_ssid->valuestring) < 4) {
    cJSON_Delete(root);
    apply_fallback(ssid_out, ssid_len, psk_out, psk_len,
      "load_wifi_credentials: SSID too short (< 4 chars) — using fallback");
    return WIFI_CRED_LOAD_FALLBACK;
  }

  strncpy(ssid_out, j_ssid->valuestring, ssid_len - 1);
  ssid_out[ssid_len - 1] = '\0';
  strncpy(psk_out,  j_psk->valuestring,  psk_len  - 1);
  psk_out[psk_len - 1] = '\0';
  cJSON_Delete(root);

  snprintf(msg_buf, sizeof(msg_buf),
    "load_wifi_credentials: loaded SSID '%s' from %s", ssid_out, json_path);
  log_info("wpa_cred", msg_buf);
  return WIFI_CRED_LOAD_SUCCESS;
}


/* Format WPA_CONFIG_WIFI_TEMPLATE with the given ssid and psk into buf.
 * Returns 0 on success, -1 if the buffer is too small. */
static int build_wpa_wifi_string(
    char *buf, size_t len, const char *ssid, const char *psk)
{
  const int n = snprintf(buf, len, WPA_CONFIG_WIFI_TEMPLATE, ssid, psk);
  if (n < 0 || (size_t)n >= len) {
    log_error("wpa_bld", "build_wpa_wifi_string: buffer too small");
    return -1;
  }
  return 0;
}