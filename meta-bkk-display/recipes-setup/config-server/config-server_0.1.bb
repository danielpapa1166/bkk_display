SUMMARY = "BKK Display HTTP config server"
DESCRIPTION = "Minimal C-based HTTP server for initial device configuration, \
replacing the Python/Bottle setup web server. Shares the boot-mode switching \
services with the old bkk-setup-web recipe."
LICENSE = "CLOSED"

inherit cmake systemd

SRC_URI = " \
    file://include/                     \
    file://src/                         \
    file://www/                         \
"

DEPENDS = "rbuflogd cjson chttp bkk-tee bkk-common-utils network-manager"
RDEPENDS:${PN} += "rbuflogd cjson wpa-supplicant bash bkk-tee bkk-common-utils network-manager"

S = "${WORKDIR}/src"

FILES:${PN} += " \
    ${bindir}/config-server \
    ${datadir}/config-server/www/index.html \
    ${datadir}/config-server/www/styles.css \
    ${datadir}/config-server/www/app.js \
"


do_install:append() {
    # web assets
    install -d ${D}${datadir}/config-server/www
    install -m 0644 ${WORKDIR}/www/index.html \
        ${D}${datadir}/config-server/www/index.html
    install -m 0644 ${WORKDIR}/www/styles.css \
        ${D}${datadir}/config-server/www/styles.css
    install -m 0644 ${WORKDIR}/www/app.js \
        ${D}${datadir}/config-server/www/app.js
}
