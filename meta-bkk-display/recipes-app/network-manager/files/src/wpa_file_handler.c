#include "wpa_file_handler.h"
#include "wpa_file_config.h"
#include <rbuflogd/logger.h>
#include <cjson/cJSON.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>


static char WPA_CONFIG_WIFI[4096];

// ----------------------------------------------------------------------------
// local function prototypes
// ----------------------------------------------------------------------------


static int prepare_config_folder(const char *path);
static int write_config_file(
  const char *path, const char *name, const char *content);
static int build_wpa_wifi_string(char *buf, size_t len,
  const char *ssid, const char *psk);

static void log_remove_status(const char * const full_path, 
  int remove_res, int errno_status); 

// ----------------------------------------------------------------------------
// global function definitions: 
// ----------------------------------------------------------------------------


// clears Wifi Protected Access (WPA) configuration files for the given config_type
wpa_file_config_stat_t clear_wpa_config(wpa_config_type_t config_type) {
  wpa_config_t *config = NULL;
  switch (config_type) {
    case WPA_CONFIG_ACCESS_POINT:
      config = &wpa_ap_config;
      break;
    case WPA_CONFIG_WIFI_CLIENT:
      config = &wpa_wifi_config;
      break;
    default:
      return WPA_FILE_CONFIG_ERROR;
  }

  char msg_buf[256];
  char remove_path[PATH_MAX];
  const char * paths_to_remove[2] = {
      config->wpa_cfg_path,
      config->network_cfg_path
  };
  const char * names_to_remove[2] = {
      config->wpa_cfg_name,
      config->network_cfg_name
  };

  for (int i = 0; i < sizeof(paths_to_remove) / sizeof(paths_to_remove[0]); i++) {
    snprintf(remove_path, sizeof(remove_path), "%s/%s",
      paths_to_remove[i], names_to_remove[i]);

    const int remove_res = remove(remove_path);
    const int errno_status = errno;
    log_remove_status(remove_path, remove_res, errno_status);
    if(remove_res != 0 && errno_status != ENOENT) {
      return WPA_FILE_CONFIG_ERROR;
    }
  }

  return WPA_FILE_CONFIG_SUCCESS;
}


/* Load SSID and password from WPA_WIFI_CREDENTIALS_JSON.
 * Copies the values into the caller-supplied buffers.
 * Returns 0 on success, 1 when the fallback (TeveClub) is used instead.
 * Never returns a negative value — a missing/corrupt file is not fatal;
 * the caller always gets a usable credential pair. */
wifi_cred_load_status_t load_wifi_credentials(
    const char *json_path,
    char *ssid_out, size_t ssid_len,
    char *psk_out,  size_t psk_len)
{
  char msg_buf[256];
  FILE *f = fopen(json_path, "r");
  if (f == NULL) {
    // json file should have been created by config-server in API_CONFIG mode, 
    // but if it's missing/unreadable for any reason that indicates 
    // a huger problem, log error and exit 
    snprintf(msg_buf, sizeof(msg_buf),
      "load_wifi_credentials: cannot open %s (%s)",
      json_path, strerror(errno));
    
    log_error("wpa_cred", msg_buf); 
      
    return WIFI_CRED_FILE_NOT_FOUND;
  }

  /* Read the whole file into a heap buffer */
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);

    snprintf(msg_buf, sizeof(msg_buf),
      "load_wifi_credentials: fseek failed for %s (%s)",
      json_path, strerror(errno));
    log_error("wpa_cred", msg_buf);

    return WIFI_CRED_FILE_OTHER_ERROR;
  }

  const long file_size = ftell(f);
  if (file_size <= 0 || file_size > 4096) {
    fclose(f);
    snprintf(msg_buf, sizeof(msg_buf),
      "load_wifi_credentials: file empty or too large (%ld bytes) for %s",
      file_size, json_path);
    log_error("wpa_cred", msg_buf);

    return WIFI_CRED_FILE_OTHER_ERROR;
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
    snprintf(msg_buf, sizeof(msg_buf),
      "load_wifi_credentials: JSON parse error for %s", json_path);
    log_error("wpa_cred", msg_buf);
    

    return WIFI_CRED_JSON_ERROR;
  }

  const cJSON *j_ssid = cJSON_GetObjectItemCaseSensitive(
    root, "ssid");
  const cJSON *j_psk  = cJSON_GetObjectItemCaseSensitive(
    root, "password");

  if (!cJSON_IsString(j_ssid) || j_ssid->valuestring[0] == '\0' ||
      !cJSON_IsString(j_psk)  || j_psk->valuestring[0]  == '\0') {
    cJSON_Delete(root);
    snprintf(msg_buf, sizeof(msg_buf),
      "load_wifi_credentials: missing/empty fields in %s", json_path);
    log_error("wpa_cred", msg_buf);
    return WIFI_CRED_JSON_ERROR;
  }


  strncpy(ssid_out, j_ssid->valuestring, ssid_len - 1);
  ssid_out[ssid_len - 1] = '\0';
  strncpy(psk_out,  j_psk->valuestring,  psk_len  - 1);
  psk_out[psk_len - 1] = '\0';
  cJSON_Delete(root);

  /*snprintf(msg_buf, sizeof(msg_buf),
    "load_wifi_credentials: loaded SSID '%s' from %s", ssid_out, json_path);
  log_info("wpa_cred", msg_buf);*/
  return WIFI_CRED_LOAD_SUCCESS;
}


// writes Wifi Protected Access (WPA) configuration files for the given config_type
wpa_file_config_stat_t write_wpa_config(wpa_config_type_t config_type) {

  wpa_config_t *config = NULL;
  switch (config_type) {
    case WPA_CONFIG_ACCESS_POINT:
      config = &wpa_ap_config;
      break;
    case WPA_CONFIG_WIFI_CLIENT:
      config = &wpa_wifi_config;

      char ssid[SSID_MAX_LEN];
      char psk[PSK_MAX_LEN];
      wifi_cred_load_status_t load_status = load_wifi_credentials(
        WPA_WIFI_CREDENTIALS_JSON,
        ssid, sizeof(ssid),
        psk,  sizeof(psk));
      if (load_status != WIFI_CRED_LOAD_SUCCESS) {
        printf("Wifi creds are invalid on not available");
        log_error("wr cfg", "write_wpa_config: "
          "failed to load wifi credentials");
        return WPA_FILE_CONFIG_ERROR;
      } 

      snprintf(WPA_CONFIG_WIFI, sizeof(WPA_CONFIG_WIFI),
        WPA_CONFIG_WIFI_TEMPLATE, ssid, psk);
      config->wpa_cfg_str = WPA_CONFIG_WIFI;

      break;
    default:
      return WPA_FILE_CONFIG_ERROR;
  }

  const char * paths_to_create[2] = {
      config->wpa_cfg_path,
      config->network_cfg_path
  };

  const char * names_to_create[2] = {
      config->wpa_cfg_name,
      config->network_cfg_name
  };

  const char * contents_to_create[2] = {
      config->wpa_cfg_str,
      config->network_cfg_str
  };

  for (int i = 0; i < sizeof(paths_to_create) / sizeof(paths_to_create[0]); i++) {

    printf("Preparing config folder: %s\n", paths_to_create[i]);
    int res = prepare_config_folder(paths_to_create[i]);
    if (res != 0) {
      log_error("wr cfg", "write_wpa_config: "
        "failed to prepare config folder");
      return WPA_FILE_CONFIG_ERROR;
    }

    printf("Writing config file: %s/%s\n", paths_to_create[i], names_to_create[i]);
    printf("Config file contents:\n%s\n", contents_to_create[i]);
    res = write_config_file(
      paths_to_create[i],
      names_to_create[i],
      contents_to_create[i]);
    if (res != 0) {
      log_error("wr cfg", "write_wpa_config: "
        "failed to write config file");
      return WPA_FILE_CONFIG_ERROR;
    }

    printf("Config file written successfully: %s/%s\n", paths_to_create[i], names_to_create[i]);
  }


  return WPA_FILE_CONFIG_SUCCESS;

}


wpa_file_config_stat_t get_wpa_config_paths(
  wpa_config_type_t config_type,
  char *wpa_cfg_full_path_out, size_t wpa_cfg_path_len,
  char *network_cfg_full_path_out, size_t network_cfg_path_len) {

  wpa_config_t *config = NULL;
  switch (config_type) {
    case WPA_CONFIG_ACCESS_POINT:
      config = &wpa_ap_config;
      break;
    case WPA_CONFIG_WIFI_CLIENT:
      config = &wpa_wifi_config;
      break;
    default:
      return WPA_FILE_CONFIG_ERROR;
  }

  char full_path[PATH_MAX];
  int n = snprintf(
    full_path, 
    sizeof(full_path), 
    "%s/%s", config->wpa_cfg_path, config->wpa_cfg_name);

  size_t copy_len = (n < wpa_cfg_path_len ? n : wpa_cfg_path_len);
  if (copy_len > 0) {
    strncpy(wpa_cfg_full_path_out, full_path, copy_len);
    wpa_cfg_full_path_out[copy_len] = '\0';
  } 
  else {
    wpa_cfg_full_path_out[0] = '\0';
  }

  n = snprintf(
    full_path, 
    sizeof(full_path), 
    "%s/%s", config->network_cfg_path, config->network_cfg_name);

  copy_len = (n < network_cfg_path_len ? n : network_cfg_path_len);
  if (copy_len > 0) {
    strncpy(network_cfg_full_path_out, full_path, copy_len);
    network_cfg_full_path_out[copy_len] = '\0';
  }
  else {
    network_cfg_full_path_out[0] = '\0';
  }

  return WPA_FILE_CONFIG_SUCCESS;
}



// ----------------------------------------------------------------------------
// local function implementations
// ----------------------------------------------------------------------------

static int prepare_config_folder(const char *path) {
  char msg_buf[256];
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
      "prepare_config_folder: "
      "path exists but is not a directory: %s", path);
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

  if(errno == EEXIST) {
    snprintf(
      msg_buf, 
      sizeof(msg_buf), 
      "prepare_config_folder: directory already exists: %s", path);
    log_debug("wr cfg", msg_buf);
  }
  else {
    snprintf(
      msg_buf, 
      sizeof(msg_buf), 
      "prepare_config_folder: created directory: %s", path);
    log_debug("wr cfg", msg_buf);
  }

  return 0;
}


static int write_config_file(
    const char *path, const char *name, const char *content) {

  char msg_buf[256];
  if (path == NULL || name == NULL || content == NULL) {
    fprintf(stderr, "write_file: NULL argument\n");
    log_error("wr cfg", "write_file: NULL argument");
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


/* Format WPA_CONFIG_WIFI_TEMPLATE with the given ssid and psk into buf.
 * Returns 0 on success, -1 if the buffer is too small. */
static int build_wpa_wifi_string(
    char *buf, size_t len, const char *ssid, const char *psk)
{
  const int n = snprintf(buf, len, 
    WPA_CONFIG_WIFI_TEMPLATE, ssid, psk);
  if (n < 0 || (size_t)n >= len) {
    log_error("wpa_bld", "build_wpa_wifi_string: buffer too small");
    return -1;
  }
  return 0;
}


static void log_remove_status(const char * const full_path, 
  int remove_res, int errno_status) {

  char msg_buf[256];
  if (remove_res != 0 && errno_status != ENOENT) {
    snprintf(msg_buf, sizeof(msg_buf),
      "log_remove_status: failed to remove %s: %s",
      full_path, strerror(errno_status));
    log_error("wr cfg", msg_buf);
  }

  if(errno_status == ENOENT) {
    snprintf(msg_buf, sizeof(msg_buf),
      "log_remove_status: file does not exist: %s", full_path);
    log_warning("wr cfg", msg_buf);
  }
  else {
    snprintf(msg_buf, sizeof(msg_buf),
      "log_remove_status: removed file: %s", full_path);
    log_debug("wr cfg", msg_buf);
  }
}