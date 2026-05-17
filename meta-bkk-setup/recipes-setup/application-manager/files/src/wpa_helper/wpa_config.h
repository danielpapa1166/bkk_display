#ifndef WPA_CONFIG_H
#define WPA_CONFIG_H

static const char WPA_CONFIG_AP[] =
    "ctrl_interface=/run/wpa_supplicant\n"
    "update_config=1\n"
    "country=HU\n"
    "\n"
    "network={\n"
    "    ssid=\"BKK-Display-Setup\"\n"
    "    mode=2\n"
    "    key_mgmt=NONE\n"
    "    frequency=2437\n"
    "}";

static const char NETWORK_CONFIG_AP[] =
    "[Match]\n"
    "Name=wlan0\n"
    "\n"
    "[Network]\n"
    "Address=192.168.4.1/24\n"
    "DHCPServer=yes\n"
    "\n"
    "[DHCPServer]\n"
    "PoolOffset=10\n"
    "PoolSize=50";

#endif /* WPA_CONFIG_H */