LICENSE = "CLOSED"

inherit cmake

SRC_URI = "file://CMakeLists.txt        \
           file://content_main.cpp      \
           file://bkk_api_client.cpp    \
           file://bkk_api_client.hpp    \
           "

S = "${WORKDIR}"

DEPENDS = "bkk-screen-owner bkk-api rbuflogd"
RDEPENDS:${PN} += "bkk-screen-owner bkk-api bkk-api-client bkk-api-keyenv rbuflogd"