#!/bin/sh
# bkk-boot-mode-restore.sh
#
# Runs on every normal (non-AP) boot to clean up any AP-mode state left
# from phase 1 of setup.
#
# Called for both phase-2 (API config) and fully-configured boots so that
# /run-based AP artefacts are always removed (they are on a tmpfs and
# disappear on reboot anyway, but we clean up explicitly for clarity).

CONFIGURED_FLAG="/etc/bkk-display-config/api-configured"
WIFI_CONFIGURED_FLAG="/etc/bkk-display-config/wifi-configured"

# Only relevant when at least WiFi has been configured (phase 2 or normal).
if [ ! -f "$WIFI_CONFIGURED_FLAG" ] && [ ! -f "$CONFIGURED_FLAG" ]; then
    exit 0
fi

# Restore client networkd config if it was moved aside during an AP boot.
# bkk-boot-mode.sh already handles this for phase-2 boots, but cover the
# case where a phase-1 boot was interrupted before the flag was written.
if [ -f /etc/systemd/network/10-wlan0.network.bak ] && \
   [ ! -f /etc/systemd/network/10-wlan0.network ]; then
    mv /etc/systemd/network/10-wlan0.network.bak \
       /etc/systemd/network/10-wlan0.network
    echo "bkk-boot-mode-restore: Restored client network config."
fi

# Clean up any leftover AP runtime config.
rm -f /run/wpa_supplicant-ap.conf
rm -f /run/systemd/network/20-wlan0-ap.network

exit 0
