LICENSE = "CLOSED"

inherit cmake pkgconfig

SRC_URI = " \
    file://CMakeLists.txt               \
    file://include                       \
    file://src                          \
    file://test                         \
"

S = "${WORKDIR}"

DEPENDS = " rbuflogd cjson systemd bkk-common-utils"
RDEPENDS:${PN} += " rbuflogd cjson libsystemd bkk-common-utils"
RDEPENDS:${PN}-dev += " bkk-common-utils-dev"

