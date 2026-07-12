LICENSE = "CLOSED" 

SRC_URI = "file://CMakeLists.txt        \
           file://timing                \
           "

S = "${WORKDIR}"

inherit cmake pkgconfig