SUMMARY = "BKK Display Application Manager"
DESCRIPTION = "Single process that owns the device lifecycle state machine, \
spawns child applications per phase, manages display ownership, and \
provides a health-check control socket."
LICENSE = "CLOSED"

inherit cmake systemd

SRC_URI = " \
    file://src/CMakeLists.txt \
    file://src/am_main.c \
"

DEPENDS = "rbuflogd cjson"
RDEPENDS:${PN} += "rbuflogd cjson"

S = "${WORKDIR}/src"

# Generate compile_commands.json for clangd tooling.
EXTRA_OECMAKE:append = " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

FILES:${PN} += "${bindir}/application_manager"
