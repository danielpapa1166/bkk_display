SUMMARY = "BKK Display Application Manager"
DESCRIPTION = "Single process that owns the device lifecycle state machine, \
spawns child applications per phase, manages display ownership, and \
provides a health-check control socket."
LICENSE = "CLOSED"

inherit cmake systemd

SRC_URI = " \
    file://src/CMakeLists.txt               \
    file://src/am_main.c                    \
    file://src/am_config_parser.c           \
    file://src/am_config_parser.h           \
    file://src/am_launcher.c                \
    file://src/am_launcher.h                \
    file://src/am_types.h                   \
    file://app_cfg/configuration.json       \
    file://application-manager.service      \
"

DEPENDS = "rbuflogd cjson"
RDEPENDS:${PN} += "rbuflogd cjson"

S = "${WORKDIR}/src"

# Generate compile_commands.json for clangd tooling.
EXTRA_OECMAKE:append = " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

FILES:${PN} += "${bindir}/application_manager"
# configuration.json is installed to /etc/application-manager/configuration.json
FILES:${PN} += "${sysconfdir}/application-manager/configuration.json"
FILES:${PN} += "${systemd_system_unitdir}/application-manager.service"

SYSTEMD_SERVICE:${PN} = "application-manager.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}${sysconfdir}/application-manager
    install -m 0644 ${WORKDIR}/app_cfg/configuration.json \
        ${D}${sysconfdir}/application-manager/configuration.json

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/application-manager.service \
        ${D}${systemd_system_unitdir}/application-manager.service
}
