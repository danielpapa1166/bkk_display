LICENSE = "CLOSED" 

SRC_URI = "file://CMakeLists.txt        \
           file://timing                \
           file://online_status         \
           file://screen_backlight      \
           file://ipc_uds               \
           file://dbus                  \
           "

S = "${WORKDIR}"

inherit cmake pkgconfig

DEPENDS += "curl dbus"