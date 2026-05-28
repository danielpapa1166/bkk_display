#!/bin/sh
# bkk-boot-mode.sh
#
# Runs early at boot to select between three operating modes:
#
#   Phase 1 (AP / WiFi config):
#     Neither flag exists -> no WiFi credentials saved yet.
#     Start an Access Point so the user can provide WiFi credentials.
#
#   Phase 2 (client / API config):
#     WIFI_CONFIGURED_FLAG exists but CONFIGURED_FLAG does not.
#     WiFi credentials were saved on the previous boot; connect to the
#     network and let the user configure the BKK API key and stations.
#
#   Normal operation:
#     CONFIGURED_FLAG exists -> fully configured, nothing to do.

CONFIGURED_FLAG="/etc/bkk-display-config/api-configured"
WIFI_CONFIGURED_FLAG="/etc/bkk-display-config/wifi-configured"

# --- fully configured: normal client boot --------------------------------
if [ -f "$CONFIGURED_FLAG" ]; then
    echo "bkk-boot-mode: Device is configured. Normal client mode."
    exit 0
fi

# --- phase 2: WiFi saved, connect and configure API ----------------------
if [ -f "$WIFI_CONFIGURED_FLAG" ]; then
    echo "bkk-boot-mode: WiFi configured. Entering client mode for API setup."
    # Restore the normal networkd client config so wlan0 connects using
    # the saved wpa_supplicant credentials.
    if [ -f /etc/systemd/network/10-wlan0.network.bak ] && \
       [ ! -f /etc/systemd/network/10-wlan0.network ]; then
        mv /etc/systemd/network/10-wlan0.network.bak \
           /etc/systemd/network/10-wlan0.network
        echo "bkk-boot-mode: Restored client network config."
    fi
    exit 0
fi

# --- phase 1: no credentials yet, start AP --------------------------------
echo "bkk-boot-mode: No WiFi config found. Entering AP setup mode."

# Stop the normal WiFi client services
systemctl stop wpa_supplicant@wlan0.service 2>/dev/null || true
systemctl stop wpa_supplicant.service 2>/dev/null || true

# Install AP-mode wpa_supplicant config
cat > /run/wpa_supplicant-ap.conf << 'EOF'
ctrl_interface=/run/wpa_supplicant
update_config=1
country=HU

network={
    ssid="BKK-Display-Setup"
    mode=2
    key_mgmt=NONE
    frequency=2437
}
EOF

# Install Access Point mode networkd config (static IP + DHCP server)
mkdir -p /run/systemd/network
cat > /run/systemd/network/20-wlan0-ap.network << 'NETEOF'
[Match]
Name=wlan0

[Network]
Address=192.168.4.1/24
DHCPServer=yes

[DHCPServer]
PoolOffset=10
PoolSize=50
NETEOF

# Mask the client network config during this boot
# (move to .bak so networkd ignores it)
if [ -f /etc/systemd/network/10-wlan0.network ]; then
    mv /etc/systemd/network/10-wlan0.network /etc/systemd/network/10-wlan0.network.bak
fi

# networkd will start after this service and pick up the AP config automatically

echo "bkk-boot-mode: AP mode active. SSID=BKK-Display-Setup IP=192.168.4.1"
