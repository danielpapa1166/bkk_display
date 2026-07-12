#!/bin/sh

echo "Updating screen components on target device..."
PROJECT_PATH="/data/projects/bkk_display/build-rpi/tmp/work/cortexa72-poky-linux/"

OWNER_PATH="$PROJECT_PATH/bkk-screen-owner/1.0-r0/"
INFO_BAR_PATH="$PROJECT_PATH/bkk-screen-info-bar/1.0-r0/"
MAIN_CONTENT_PATH="$PROJECT_PATH/bkk-screen-main-content/1.0-r0/"

UTILS_LIB_PATH="$PROJECT_PATH/bkk-common-utils/1.0-r0/"

BIN_PATH="usr/bin/"
INC_PATH="usr/include/"
LIB_PATH="usr/lib/"

TARGET="root@192.168.0.50"

echo "Project path: $PROJECT_PATH"
echo "Update owner files..."
scp -r "$OWNER_PATH"/package/usr/bin/bkk-screen-owner  "$TARGET":/"$BIN_PATH"
scp -r "$OWNER_PATH"/package/usr/include/bkk_screen_client  "$TARGET":/"$INC_PATH"
scp -r "$OWNER_PATH"/package/usr/lib/libbkk-screen-client.so "$TARGET":/"$LIB_PATH"
scp -r "$OWNER_PATH"/package/usr/lib/libbkk-screen-client.so.1 "$TARGET":/"$LIB_PATH"


scp -r "$INFO_BAR_PATH"/package/usr/bin/bkk_screen_info_bar "$TARGET":/"$BIN_PATH"
scp -r "$MAIN_CONTENT_PATH"/package/usr/bin/bkk_screen_main_content "$TARGET":/"$BIN_PATH"

scp -r "$UTILS_LIB_PATH"/package/usr/lib/libbkk_utils_timing.so "$TARGET":/"$LIB_PATH"
scp -r "$UTILS_LIB_PATH"/package/usr/lib/libbkk_utils_timing.so.1 "$TARGET":/"$LIB_PATH"
scp -r "$UTILS_LIB_PATH"/package/usr/lib/libbkk_utils_timing.so.1.0.0 "$TARGET":/"$LIB_PATH"
scp -r "$UTILS_LIB_PATH"/package/usr/include/bkk_utils "$TARGET":/"$INC_PATH"


echo "Update complete."
