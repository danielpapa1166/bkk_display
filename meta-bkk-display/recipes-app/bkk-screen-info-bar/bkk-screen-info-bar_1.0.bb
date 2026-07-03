LICENSE = "CLOSED"

inherit cmake

SRC_URI = "file://CMakeLists.txt        \
           file://info_bar_main.cpp     \
           file://clock_update.cpp      \
           file://clock_update.hpp      \
           "

S = "${WORKDIR}"

DEPENDS = "bkk-screen-owner"
RDEPENDS:${PN} += " bkk-screen-owner"