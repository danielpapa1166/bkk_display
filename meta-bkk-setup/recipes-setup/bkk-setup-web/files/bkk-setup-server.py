#!/usr/bin/env python3
"""
test python script for bkk web setup 
"""

import os
import sys
import subprocess

# Add script directory to path so vendored bottle.py is found
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bottle import Bottle, request, response

CONFIGURED_FLAG = "/etc/bkk-config/configured"

app = Bottle()

HTML_PAGE = """\
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>BKK Display Setup</title>
</head>
<body>
    <div class="card">
        <h1>BKK Display Setup</h1>
        <div id="content">
            <button id="btn" onclick="configure()">Config</button>
        </div>
    </div>
    <script>
        function configure() {
            var btn = document.getElementById('btn');
            btn.disabled = true;
            btn.textContent = 'Configuring...';
            fetch('/api/finish', { method: 'POST' })
                .then(function(r) { return r.json(); })
                .then(function(data) {
                    document.getElementById('content').innerHTML =
                        '<p class="done">Done! Device will reboot now...</p>';
                })
                .catch(function(err) {
                    btn.disabled = false;
                    btn.textContent = 'Config';
                    alert('Error: ' + err);
                });
        }
    </script>
</body>
</html>
"""


@app.route("/")
def index():
    return HTML_PAGE


@app.route("/api/finish", method="POST")
def finish():
    os.makedirs(os.path.dirname(CONFIGURED_FLAG), exist_ok=True)
    with open(CONFIGURED_FLAG, "w") as f:
        f.write("1\n")
    # Schedule reboot in 2 seconds so we can respond first
    subprocess.Popen(["sh", "-c", "sleep 2 && reboot"])
    response.content_type = "application/json"
    return '{"status": "ok", "message": "Rebooting..."}'


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080, quiet=False)
