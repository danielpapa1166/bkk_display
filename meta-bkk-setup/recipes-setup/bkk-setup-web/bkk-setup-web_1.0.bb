SUMMARY = "BKK Display first-boot setup web server and boot mode switching"
DESCRIPTION = "Provides AP-mode hotspot + Bottle web server for initial device configuration, \
and a boot-mode selector that switches between setup and normal operation."
HOMEPAGE = "https://example.local/bkk-setup-web"
LICENSE = "CLOSED"

SRC_URI = " \
    file://bottle.py \
    file://bkk-setup-server.py \
    file://bkk-boot-mode.sh \
    file://bkk-boot-mode-restore.sh \
    file://bkk-boot-mode.service \
    file://bkk-boot-mode-restore.service \
    file://bkk-setup-web.service \
"

S = "${WORKDIR}"

inherit systemd

RDEPENDS:${PN} += "python3 bash wpa-supplicant"

FILES:${PN} += " \
    ${systemd_system_unitdir}/bkk-boot-mode.service \
    ${systemd_system_unitdir}/bkk-boot-mode-restore.service \
    ${systemd_system_unitdir}/bkk-setup-web.service \
    ${libexecdir}/bkk-setup/bottle.py \
    ${libexecdir}/bkk-setup/bkk-setup-server.py \
    ${libexecdir}/bkk-setup/bkk-boot-mode.sh \
    ${libexecdir}/bkk-setup/bkk-boot-mode-restore.sh \
"

SYSTEMD_SERVICE:${PN} = " \
    bkk-boot-mode.service \
    bkk-boot-mode-restore.service \
    bkk-setup-web.service \
"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install() {
    # systemd units
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/bkk-boot-mode.service \
        ${D}${systemd_system_unitdir}/bkk-boot-mode.service
    install -m 0644 ${WORKDIR}/bkk-boot-mode-restore.service \
        ${D}${systemd_system_unitdir}/bkk-boot-mode-restore.service
    install -m 0644 ${WORKDIR}/bkk-setup-web.service \
        ${D}${systemd_system_unitdir}/bkk-setup-web.service

    # scripts
    install -d ${D}${libexecdir}/bkk-setup
    install -m 0644 ${WORKDIR}/bottle.py \
        ${D}${libexecdir}/bkk-setup/bottle.py
    install -m 0755 ${WORKDIR}/bkk-setup-server.py \
        ${D}${libexecdir}/bkk-setup/bkk-setup-server.py
    install -m 0755 ${WORKDIR}/bkk-boot-mode.sh \
        ${D}${libexecdir}/bkk-setup/bkk-boot-mode.sh
    install -m 0755 ${WORKDIR}/bkk-boot-mode-restore.sh \
        ${D}${libexecdir}/bkk-setup/bkk-boot-mode-restore.sh
}
