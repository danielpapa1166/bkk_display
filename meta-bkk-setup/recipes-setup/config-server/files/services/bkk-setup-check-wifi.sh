#!/bin/sh
# bkk-setup-check-wifi.sh
#
# Phase-2 connectivity check.  Runs after systemd-networkd brings up wlan0
# using the credentials saved during phase 1.
#
# Success: exits 0 so that bkk-setup-api.service can start.
# Failure: removes /etc/bkk-display-config/wifi-configured and reboots back to
#          phase 1 so the user can correct the WiFi credentials.

WIFI_CONFIGURED_FLAG="/etc/bkk-display-config/wifi-configured"
PING_TARGET="8.8.8.8"
MAX_ATTEMPTS=3

i=1
while [ "$i" -le "$MAX_ATTEMPTS" ]; do
    echo "bkk-setup-check-wifi: Ping attempt $i/$MAX_ATTEMPTS ..."
    if ping -c 1 -W 5 "$PING_TARGET" > /dev/null 2>&1; then
        echo "bkk-setup-check-wifi: Network reachable. Proceeding to API setup."
        exit 0
    fi
    i=$((i + 1))
done

echo "bkk-setup-check-wifi: No connectivity after $MAX_ATTEMPTS attempts."
echo "bkk-setup-check-wifi: Rolling back to phase 1 (AP mode) on next boot."
rm -f "$WIFI_CONFIGURED_FLAG"
systemctl reboot
