LICENSE = "CLOSED"

inherit cmake

SRC_URI = "file://CMakeLists.txt        \
           file://info_bar_main.cpp     \
           "

S = "${WORKDIR}"

DEPENDS = "bkk-screen-owner"
RDEPENDS:${PN} += " bkk-screen-owner"