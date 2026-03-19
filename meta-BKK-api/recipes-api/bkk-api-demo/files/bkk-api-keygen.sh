#!/bin/bash
# Read BKK API key from file and export as environment variable
# This script is run as a oneshot service to set up the API key environment

KEY_FILE="/etc/bkk-api/api-key.txt"

if [ ! -f "$KEY_FILE" ]; then
    echo "Error: BKK API key file not found at $KEY_FILE" >&2
    exit 1
fi

# Read the key from file (remove any trailing whitespace)
API_KEY=$(cat "$KEY_FILE" | tr -d '\n' | sed 's/[[:space:]]*$//')

if [ -z "$API_KEY" ]; then
    echo "Error: BKK API key is empty" >&2
    exit 1
fi

# Write to systemd environment directory so it's available to all services
mkdir -p /etc/environment.d
cat > /etc/environment.d/10-bkk-api.conf <<EOF
BKK_API_KEY=$API_KEY
EOF

chmod 0644 /etc/environment.d/10-bkk-api.conf

echo "BKK API key environment variable set successfully"
exit 0
