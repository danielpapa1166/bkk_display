LICENSE = "CLOSED"

inherit cmake

SRC_URI = " \
    file://CMakeLists.txt               \
    file://src                          \
"

S = "${WORKDIR}"

DEPENDS = " rbuflogd cjson systemd"
RDEPENDS:${PN} += " rbuflogd cjson libsystemd"

