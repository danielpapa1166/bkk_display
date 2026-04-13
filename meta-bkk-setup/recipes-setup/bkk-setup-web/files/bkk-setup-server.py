#!/usr/bin/env python3
"""
test python script for bkk web setup 
"""

import os
import sys
import json
import subprocess

# Add script directory to path so vendored bottle.py is found
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
from bottle import Bottle, request, response, static_file

BKK_CONFIG_DIR = "/etc/bkk-api"
CONFIGURED_FLAG = os.path.join(BKK_CONFIG_DIR, "configured")
CONFIG_FILE = os.path.join(BKK_CONFIG_DIR, "config.json")
KEY_FILE = os.path.join(BKK_CONFIG_DIR, "api-key.txt")
WPA_SUPPLICANT_FILE_WLAN0 = "/etc/wpa_supplicant/wpa_supplicant-wlan0.conf"
WPA_SUPPLICANT_FILE = "/etc/wpa_supplicant/wpa_supplicant.conf"

app = Bottle()


def _escape_wpa_value(value):
    # Keep generated config valid by escaping quotes and backslashes.
    return str(value).replace("\\", "\\\\").replace('"', '\\"')


def write_wpa_supplicant_config(ssid, password):
    content = (
        "ctrl_interface=/run/wpa_supplicant\n"
        "ctrl_interface_group=wheel\n"
        "update_config=1\n"
        "country=HU\n\n"
        "network={\n"
        f'    ssid="{_escape_wpa_value(ssid)}"\n'
        f'    psk="{_escape_wpa_value(password)}"\n'
        "    key_mgmt=WPA-PSK\n"
        "}\n"
    )

    with open(WPA_SUPPLICANT_FILE, "w") as f:
        f.write(content)

    with open(WPA_SUPPLICANT_FILE_WLAN0, "w") as f:
        f.write(content)

    os.chmod(WPA_SUPPLICANT_FILE, 0o600)
    os.chmod(WPA_SUPPLICANT_FILE_WLAN0, 0o600)


def read_stored_api_key():
    if not os.path.exists(KEY_FILE):
        return ""

    try:
        with open(KEY_FILE, "r") as f:
            return f.read().strip()
    except OSError:
        return ""


def write_api_key(api_key):
    with open(KEY_FILE, "w") as f:
        f.write(api_key + "\n")

    os.chmod(KEY_FILE, 0o600)


@app.route("/")
def index():
    return static_file("index.html", root=SCRIPT_DIR)


@app.route("/static/<filename>")
def serve_static(filename):
    return static_file(filename, root=SCRIPT_DIR)


@app.route("/api/finish", method="POST")
def finish():
    data = request.json or {}

    wifi_ssid = data.get("wifi", {}).get("ssid", "").strip()
    wifi_password = data.get("wifi", {}).get("password", "")
    submitted_api_key = data.get("api_key", "").strip()
    stored_api_key = read_stored_api_key()
    final_api_key = submitted_api_key if submitted_api_key else stored_api_key

    config = {
        "wifi": {
            "ssid": wifi_ssid,
            "password": wifi_password,
        },
        "api_key": final_api_key,
        "stations": data.get("stations", []),
    }

    os.makedirs(BKK_CONFIG_DIR, exist_ok=True)

    if wifi_ssid and wifi_password:
        write_wpa_supplicant_config(wifi_ssid, wifi_password)

    if submitted_api_key:
        write_api_key(submitted_api_key)

    with open(CONFIG_FILE, "w") as f:
        json.dump(config, f, indent=2)

    with open(CONFIGURED_FLAG, "w") as f:
        f.write("1\n")

    # Schedule reboot in 2 seconds so we can respond first
    subprocess.Popen(["sh", "-c", "sleep 2 && reboot"])
    response.content_type = "application/json"
    return '{"status": "ok", "message": "Rebooting..."}'


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080, quiet=False)
