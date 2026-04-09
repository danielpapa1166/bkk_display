#!/bin/sh
# bkk-boot-mode.sh
#
# Runs early at boot to decide between AP (setup) mode and client (normal) mode.
# - If /etc/bkk-config/configured exists -> client mode (do nothing, default services handle it)
# - If not -> switch wlan0 to AP mode for first-boot setup

CONFIGURED_FLAG="/etc/bkk-config/configured"

if [ -f "$CONFIGURED_FLAG" ]; then
    echo "bkk-boot-mode: Device is configured. Normal client mode."
    exit 0
fi

echo "bkk-boot-mode: Device NOT configured. Entering AP setup mode."

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

# Install AP-mode networkd config (static IP + DHCP server)
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
