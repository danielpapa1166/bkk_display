#!/bin/sh

echo "Updating WEB config files on target device..."
PROJECT_PATH="/data/projects/bkk_display/build-rpi/tmp/work/cortexa72-poky-linux"
SOURCES_PATH="/data/projects/bkk_display/meta-bkk-display/recipes-setup/config-server/files/www"

CONFIG_SERVER_PATH="$PROJECT_PATH/config-server/0.1-r0/"


BIN_PATH="usr/bin/"
INC_PATH="usr/include/"
LIB_PATH="usr/lib/"
RES_PATH="/usr/share/config-server/www/"

TARGET="root@192.168.0.50"

echo "Project path: $PROJECT_PATH"
echo "Update owner files..."

scp -r "$CONFIG_SERVER_PATH"package/usr/bin/config-server  "$TARGET":"/$BIN_PATH"
scp -r "$SOURCES_PATH"/*  "$TARGET":"/$RES_PATH"