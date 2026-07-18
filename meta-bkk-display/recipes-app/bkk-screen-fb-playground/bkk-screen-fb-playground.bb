LICENSE = "CLOSED"

inherit cmake pkgconfig

SRC_URI = "file://CMakeLists.txt        \
           file://main.c                \
           "

S = "${WORKDIR}"