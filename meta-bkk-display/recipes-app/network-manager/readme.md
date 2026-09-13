# Network Manager

`network-manager` is the project-specific network configuration executable.
It manages wpa_supplicant configuration and networkd status while presenting a
small public interface for other BKK components.

## Architecture

`network_manager` is built from four C modules:

- `nm_main.c` is the executable entry point.
- `wpa_file_handler.c` reads and writes WPA configuration.
- `supplicant_handler.c` interacts with wpa_supplicant.
- `networkd_status.c` obtains systemd-networkd state.

It uses cJSON for structured data, libsystemd, rbuflogd, and the common D-Bus
utilities. `include/network_manager_pub.h` is installed as its public API.

## Integration

The `network-manager.bb` recipe installs the executable and public header. It
builds against systemd and `bkk-common-utils`; its runtime package requires
libsystemd, cJSON, rbuflogd, and the common utilities. Setup-oriented services
such as `config-server` depend on it to apply network configuration.