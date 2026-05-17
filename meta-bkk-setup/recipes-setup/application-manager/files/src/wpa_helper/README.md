# wpa_helper

Configures wpa_supplicant and systemd-networkd for the BKK Display setup modes.

## Usage

```
wpa_helper boot_mode=<mode> [key=value ...]
```

### Mandatory argument

| Argument | Values | Description |
|---|---|---|
| `boot_mode` | `wifi_config`, `api_config`, `normal` | Selects the operating mode |

### Optional arguments

| Argument | Description |
|---|---|
| `wpa_cfg_path` | Directory where the wpa_supplicant config file will be written |
| `network_cfg_path` | Directory where the systemd-networkd config file will be written |
| `wpa_cfg_name` | Override the wpa_supplicant config filename |
| `network_cfg_name` | Override the systemd-networkd config filename |

### Examples

```sh
wpa_helper boot_mode=wifi_config
wpa_helper boot_mode=wifi_config wpa_cfg_path=/etc/wpa_supplicant network_cfg_path=/etc/systemd/network
```
