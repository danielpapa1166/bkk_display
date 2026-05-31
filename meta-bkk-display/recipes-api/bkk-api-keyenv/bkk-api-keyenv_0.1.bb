SUMMARY = "BKK API key provisioning helpers"
DESCRIPTION = "Installs the BKK API key file and the systemd bootstrap service that exports it to the runtime environment"
HOMEPAGE = "https://example.local/bkk-api-keyenv"
LICENSE = "CLOSED"

SRC_URI = " \
    file://bkk-api-keygen.sh \
    file://bkk-api-keygen.service \
    file://api-key.txt \
"

S = "${WORKDIR}"

inherit systemd

RDEPENDS:${PN} += "bash"

FILES:${PN} += "${systemd_system_unitdir}/bkk-api-keygen.service"
FILES:${PN} += "${libexecdir}/bkk-api/bkk-api-keygen.sh"
FILES:${PN} += "${sysconfdir}/bkk-api/api-key.txt"
CONFFILES:${PN} += "${sysconfdir}/bkk-api/api-key.txt"

SYSTEMD_SERVICE:${PN} = "bkk-api-keygen.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/bkk-api-keygen.service ${D}${systemd_system_unitdir}/bkk-api-keygen.service

    install -d ${D}${libexecdir}/bkk-api
    install -m 0755 ${WORKDIR}/bkk-api-keygen.sh ${D}${libexecdir}/bkk-api/bkk-api-keygen.sh

    install -d ${D}${sysconfdir}/bkk-api
    install -m 0640 ${WORKDIR}/api-key.txt ${D}${sysconfdir}/bkk-api/api-key.txt
}