SUMMARY = "BKK Display HTTP config server"
DESCRIPTION = "Minimal C-based HTTP server for initial device configuration, \
replacing the Python/Bottle setup web server. Shares the boot-mode switching \
services with the old bkk-setup-web recipe."
LICENSE = "CLOSED"

inherit cmake systemd

SRC_URI = " \
    file://src/CMakeLists.txt \
    file://src/http_server_main.c \
    file://src/http_server_config.h \
    file://src/http_server_client_handler.c \
    file://src/http_server_client_handler.h \
    file://src/http_server_post_handler.c \
    file://src/http_server_post_handler.h \
    file://src/http_server_resource_handler.c \
    file://src/http_server_resource_handler.h \
    file://src/http_server_user_action_handler.c \
    file://src/http_server_user_action_handler.h \
    file://src/http_server_wifi_validation.c \
    file://src/http_server_wifi_validation.h \
    file://src/http_server_utils.h \
    file://www/index.html \
    file://www/styles.css \
    file://www/app.js \
    file://services/bkk-boot-mode.sh \
    file://services/bkk-boot-mode-restore.sh \
    file://services/bkk-setup-check-wifi.sh \
    file://services/bkk-boot-mode.service \
    file://services/bkk-boot-mode-restore.service \
    file://services/bkk-setup-web.service \
    file://services/bkk-setup-check-wifi.service \
    file://services/bkk-setup-api.service \
"

DEPENDS = "rbuflogd cjson"
RDEPENDS:${PN} += "rbuflogd cjson wpa-supplicant bash"

S = "${WORKDIR}/src"

FILES:${PN} += " \
    ${bindir}/config-server \
    ${datadir}/config-server/www/index.html \
    ${datadir}/config-server/www/styles.css \
    ${datadir}/config-server/www/app.js \
    ${systemd_system_unitdir}/bkk-boot-mode.service \
    ${systemd_system_unitdir}/bkk-boot-mode-restore.service \
    ${systemd_system_unitdir}/bkk-setup-web.service \
    ${systemd_system_unitdir}/bkk-setup-check-wifi.service \
    ${systemd_system_unitdir}/bkk-setup-api.service \
    ${libexecdir}/bkk-setup/bkk-boot-mode.sh \
    ${libexecdir}/bkk-setup/bkk-boot-mode-restore.sh \
    ${libexecdir}/bkk-setup/bkk-setup-check-wifi.sh \
"

SYSTEMD_SERVICE:${PN} = " \
    bkk-boot-mode.service \
    bkk-boot-mode-restore.service \
    bkk-setup-web.service \
    bkk-setup-check-wifi.service \
    bkk-setup-api.service \
"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    # web assets
    install -d ${D}${datadir}/config-server/www
    install -m 0644 ${WORKDIR}/www/index.html \
        ${D}${datadir}/config-server/www/index.html
    install -m 0644 ${WORKDIR}/www/styles.css \
        ${D}${datadir}/config-server/www/styles.css
    install -m 0644 ${WORKDIR}/www/app.js \
        ${D}${datadir}/config-server/www/app.js

    # systemd units
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/services/bkk-boot-mode.service \
        ${D}${systemd_system_unitdir}/bkk-boot-mode.service
    install -m 0644 ${WORKDIR}/services/bkk-boot-mode-restore.service \
        ${D}${systemd_system_unitdir}/bkk-boot-mode-restore.service
    install -m 0644 ${WORKDIR}/services/bkk-setup-web.service \
        ${D}${systemd_system_unitdir}/bkk-setup-web.service
    install -m 0644 ${WORKDIR}/services/bkk-setup-check-wifi.service \
        ${D}${systemd_system_unitdir}/bkk-setup-check-wifi.service
    install -m 0644 ${WORKDIR}/services/bkk-setup-api.service \
        ${D}${systemd_system_unitdir}/bkk-setup-api.service

    # boot-mode and setup scripts
    install -d ${D}${libexecdir}/bkk-setup
    install -m 0755 ${WORKDIR}/services/bkk-boot-mode.sh \
        ${D}${libexecdir}/bkk-setup/bkk-boot-mode.sh
    install -m 0755 ${WORKDIR}/services/bkk-boot-mode-restore.sh \
        ${D}${libexecdir}/bkk-setup/bkk-boot-mode-restore.sh
    install -m 0755 ${WORKDIR}/services/bkk-setup-check-wifi.sh \
        ${D}${libexecdir}/bkk-setup/bkk-setup-check-wifi.sh
}
