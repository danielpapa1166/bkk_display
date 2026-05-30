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
    file://src/am_supervisor.c              \
    file://src/am_supervisor.h              \
    file://src/am_boot_mode.c               \
    file://src/am_boot_mode.h               \
    file://src/am_types.h                   \
    file://src/am_http_server.c             \
    file://src/am_http_server.h             \
    file://www/index.html                   \
    file://www/style.css                    \
    file://www/app.js                       \
    file://app_cfg/configuration.json       \
    file://application-manager.service      \
    file://src/wpa_helper/CMakeLists.txt         \
    file://src/wpa_helper/wpa_helper_main.c      \
    file://src/wpa_helper/wpa_config.h           \
    file://src/logger_check/CMakeLists.txt            \
    file://src/logger_check/logger_check_main.c       \
    file://src/networkd_check/CMakeLists.txt          \
    file://src/networkd_check/networkd_check_main.c   \
"

DEPENDS = "rbuflogd cjson chttp systemd"
RDEPENDS:${PN} += "rbuflogd cjson libsystemd"

S = "${WORKDIR}/src"

# Generate compile_commands.json for clangd tooling.
EXTRA_OECMAKE:append = " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

FILES:${PN} += "${bindir}/application_manager"
FILES:${PN} += "${bindir}/wpa_helper"
FILES:${PN} += "${bindir}/logger_check"
FILES:${PN} += "${bindir}/networkd_check"
# configuration.json is installed to /etc/application-manager/configuration.json
FILES:${PN} += "${sysconfdir}/application-manager/configuration.json"
FILES:${PN} += "${systemd_system_unitdir}/application-manager.service"
FILES:${PN} += "${datadir}/application-manager/www/index.html"
FILES:${PN} += "${datadir}/application-manager/www/style.css"
FILES:${PN} += "${datadir}/application-manager/www/app.js"

SYSTEMD_SERVICE:${PN} = "application-manager.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}${sysconfdir}/application-manager
    install -m 0644 ${WORKDIR}/app_cfg/configuration.json \
        ${D}${sysconfdir}/application-manager/configuration.json

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/application-manager.service \
        ${D}${systemd_system_unitdir}/application-manager.service

    install -d ${D}${datadir}/application-manager/www
    install -m 0644 ${WORKDIR}/www/index.html \
        ${D}${datadir}/application-manager/www/index.html
    install -m 0644 ${WORKDIR}/www/style.css \
        ${D}${datadir}/application-manager/www/style.css
    install -m 0644 ${WORKDIR}/www/app.js \
        ${D}${datadir}/application-manager/www/app.js
}
