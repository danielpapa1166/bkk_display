#!/bin/sh
# bkk-boot-mode-restore.sh
#
# Runs when the device is configured and booting in normal mode.
# Restores client network config if it was moved aside during a previous AP boot.

if [ -f /etc/systemd/network/10-wlan0.network.bak ] && \
   [ ! -f /etc/systemd/network/10-wlan0.network ]; then
    mv /etc/systemd/network/10-wlan0.network.bak /etc/systemd/network/10-wlan0.network
    echo "bkk-boot-mode-restore: Restored client network config."
fi

# Clean up any leftover AP config from /run (tmpfs, usually gone after reboot anyway)
rm -f /run/wpa_supplicant-ap.conf
rm -f /run/systemd/network/20-wlan0-ap.network

exit 0
