#include "http_server_wifi_validation.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


#define WIFI_VALIDATE_TIMEOUT_SEC           15


static int run_command_capture(const char *command, char *out, size_t out_size) {
  FILE *fp = NULL;
  size_t used = 0;
  int status = 0;

  if (command == NULL || out == NULL || out_size == 0) {
    return -1;
  }

  out[0] = '\0';
  fp = popen(command, "r");
  if (fp == NULL) {
    return -1;
  }

  while (used + 1 < out_size) {
    const size_t remaining = out_size - used - 1;
    if (fgets(out + used, (int)remaining + 1, fp) == NULL) {
      break;
    }
    used = strlen(out);
  }

  status = pclose(fp);
  if (status == -1) {
    return -1;
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    return -1;
  }

  return 0;
}

static int run_wpa_cli(const char *args, char *out, size_t out_size) {
  char command[512] = { 0 };

  if (snprintf(command, sizeof(command),
      "wpa_cli -i wlan0 -p /run/wpa_supplicant %s 2>/dev/null", args) >= (int)sizeof(command)) {
    return -1;
  }

  return run_command_capture(command, out, out_size);
}

static int shell_quote_single(const char *in, char *out, size_t out_size) {
  static const char escaped_single_quote[] = "'\\''";
  size_t out_idx = 0;

  if (in == NULL || out == NULL || out_size < 3) {
    return -1;
  }

  out[out_idx++] = '\'';
  for (size_t i = 0; in[i] != '\0'; i++) {
    if (in[i] == '\'') {
      if (out_idx + sizeof(escaped_single_quote) - 1 >= out_size) {
        return -1;
      }
      memcpy(out + out_idx, escaped_single_quote, sizeof(escaped_single_quote) - 1);
      out_idx += sizeof(escaped_single_quote) - 1;
    } else {
      if (out_idx + 1 >= out_size) {
        return -1;
      }
      out[out_idx++] = in[i];
    }
  }

  if (out_idx + 2 > out_size) {
    return -1;
  }
  out[out_idx++] = '\'';
  out[out_idx] = '\0';
  return 0;
}

static int ssid_to_hex(const char *ssid, char *out, size_t out_size) {
  static const char hex_chars[] = "0123456789abcdef";
  size_t ssid_len = 0;

  if (ssid == NULL || out == NULL || out_size == 0) {
    return -1;
  }

  ssid_len = strlen(ssid);
  if (ssid_len == 0 || (ssid_len * 2 + 1) > out_size) {
    return -1;
  }

  for (size_t i = 0; i < ssid_len; i++) {
    const unsigned char c = (unsigned char)ssid[i];
    out[i * 2] = hex_chars[(c >> 4) & 0x0F];
    out[i * 2 + 1] = hex_chars[c & 0x0F];
  }
  out[ssid_len * 2] = '\0';
  return 0;
}

static int extract_psk_hex(const char *wpa_passphrase_output, char *out_psk, size_t out_size) {
  const char *cursor = wpa_passphrase_output;

  if (wpa_passphrase_output == NULL || out_psk == NULL || out_size < 65) {
    return -1;
  }

  while (cursor != NULL && *cursor != '\0') {
    const char *line_end = strchr(cursor, '\n');
    size_t line_len = (line_end != NULL) ? (size_t)(line_end - cursor) : strlen(cursor);

    if (line_len > 4 && strncmp(cursor, "\tpsk=", 5) == 0) {
      const char *value = cursor + 5;
      size_t value_len = line_len - 5;

      if (value_len == 64 && out_size >= 65) {
        memcpy(out_psk, value, 64);
        out_psk[64] = '\0';
        return 0;
      }
    }

    if (line_end == NULL) {
      break;
    }
    cursor = line_end + 1;
  }

  return -1;
}

static int derive_psk_hex(const char *ssid, const char *password, char *out_psk, size_t out_size) {
  char quoted_ssid[300] = { 0 };
  char quoted_password[300] = { 0 };
  char command[1024] = { 0 };
  char output[4096] = { 0 };

  if (shell_quote_single(ssid, quoted_ssid, sizeof(quoted_ssid)) != 0) {
    return -1;
  }
  if (shell_quote_single(password, quoted_password, sizeof(quoted_password)) != 0) {
    return -1;
  }

  if (snprintf(command, sizeof(command), "wpa_passphrase %s %s 2>/dev/null", quoted_ssid, quoted_password)
      >= (int)sizeof(command)) {
    return -1;
  }

  if (run_command_capture(command, output, sizeof(output)) != 0) {
    return -1;
  }

  return extract_psk_hex(output, out_psk, out_size);
}

int validate_wifi_credentials(const char *ssid, const char *password) {
  char output[2048] = { 0 };
  char psk_hex[65] = { 0 };
  char ssid_hex[129] = { 0 };
  char cmd[256] = { 0 };
  int network_id = -1;

  if (ssid == NULL || password == NULL) {
    return -1;
  }
  if (ssid[0] == '\0' || password[0] == '\0') {
    return -1;
  }

  if (ssid_to_hex(ssid, ssid_hex, sizeof(ssid_hex)) != 0) {
    return -1;
  }

  if (derive_psk_hex(ssid, password, psk_hex, sizeof(psk_hex)) != 0) {
    return -1;
  }

  if (run_wpa_cli("add_network", output, sizeof(output)) != 0) {
    return -1;
  }
  network_id = atoi(output);
  if (network_id < 0) {
    return -1;
  }

  if (snprintf(cmd, sizeof(cmd), "set_network %d ssid %s", network_id, ssid_hex) >= (int)sizeof(cmd)) {
    return -1;
  }
  if (run_wpa_cli(cmd, output, sizeof(output)) != 0 || strncmp(output, "OK", 2) != 0) {
    goto cleanup;
  }

  if (snprintf(cmd, sizeof(cmd), "set_network %d psk %s", network_id, psk_hex) >= (int)sizeof(cmd)) {
    goto cleanup;
  }
  if (run_wpa_cli(cmd, output, sizeof(output)) != 0 || strncmp(output, "OK", 2) != 0) {
    goto cleanup;
  }

  if (snprintf(cmd, sizeof(cmd), "enable_network %d", network_id) >= (int)sizeof(cmd)) {
    goto cleanup;
  }
  if (run_wpa_cli(cmd, output, sizeof(output)) != 0) {
    goto cleanup;
  }

  if (snprintf(cmd, sizeof(cmd), "select_network %d", network_id) >= (int)sizeof(cmd)) {
    goto cleanup;
  }
  if (run_wpa_cli(cmd, output, sizeof(output)) != 0) {
    goto cleanup;
  }

  for (int i = 0; i < WIFI_VALIDATE_TIMEOUT_SEC; i++) {
    if (run_wpa_cli("status", output, sizeof(output)) != 0) {
      goto cleanup;
    }

    if (strstr(output, "wpa_state=COMPLETED") != NULL) {
      goto success;
    }
    sleep(1);
  }

cleanup:
  if (network_id >= 0) {
    if (snprintf(cmd, sizeof(cmd), "remove_network %d", network_id) < (int)sizeof(cmd)) {
      run_wpa_cli(cmd, output, sizeof(output));
    }
  }
  run_wpa_cli("reconfigure", output, sizeof(output));
  return -1;

success:
  if (network_id >= 0) {
    if (snprintf(cmd, sizeof(cmd), "remove_network %d", network_id) < (int)sizeof(cmd)) {
      run_wpa_cli(cmd, output, sizeof(output));
    }
  }
  run_wpa_cli("reconfigure", output, sizeof(output));
  return 0;
}
