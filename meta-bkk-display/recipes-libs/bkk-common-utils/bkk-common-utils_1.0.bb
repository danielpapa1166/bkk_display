LICENSE = "CLOSED" 

SRC_URI = "file://CMakeLists.txt        \
           file://timing                \
           file://online_status         \
           file://screen_backlight      \
           "

S = "${WORKDIR}"

inherit cmake pkgconfig

DEPENDS += "curl"