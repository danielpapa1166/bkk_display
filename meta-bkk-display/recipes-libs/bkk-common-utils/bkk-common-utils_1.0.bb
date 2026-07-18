LICENSE = "CLOSED" 

SRC_URI = "file://CMakeLists.txt        \
           file://timing                \
           file://online_status         \
           "

S = "${WORKDIR}"

inherit cmake pkgconfig

DEPENDS += "curl"