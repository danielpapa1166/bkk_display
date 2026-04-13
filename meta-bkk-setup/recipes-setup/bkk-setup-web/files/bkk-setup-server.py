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

app = Bottle()


@app.route("/")
def index():
    return static_file("index.html", root=SCRIPT_DIR)


@app.route("/static/<filename>")
def serve_static(filename):
    return static_file(filename, root=SCRIPT_DIR)


@app.route("/api/finish", method="POST")
def finish():
    data = request.json or {}

    config = {
        "wifi": {
            "ssid": data.get("wifi", {}).get("ssid", ""),
            "password": data.get("wifi", {}).get("password", ""),
        },
        "api_key": data.get("api_key", ""),
        "stations": data.get("stations", []),
    }

    os.makedirs(BKK_CONFIG_DIR, exist_ok=True)

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
