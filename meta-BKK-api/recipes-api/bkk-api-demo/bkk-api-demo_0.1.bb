SUMMARY = "Minimal BKK API demo application"
DESCRIPTION = "Small C++ app that uses BkkApi and calls a member function"
HOMEPAGE = "https://example.local/bkk-api-demo"
LICENSE = "CLOSED"

SRC_URI = " \
    file://CMakeLists.txt \
    file://main.cpp \
    file://bkk-api-demo.service \
    file://bkk-api-keygen.sh \
    file://bkk-api-keygen.service \
    file://api-key.txt \
"

S = "${WORKDIR}"

inherit cmake pkgconfig systemd

DEPENDS = "bkk-api"
RDEPENDS:${PN} += "bkk-api bash"

FILES:${PN} += "${bindir}/bkk-api-demo"
FILES:${PN} += "${systemd_system_unitdir}/bkk-api-demo.service"
FILES:${PN} += "${systemd_system_unitdir}/bkk-api-keygen.service"
FILES:${PN} += "${libexecdir}/bkk-api/bkk-api-keygen.sh"
FILES:${PN} += "${sysconfdir}/bkk-api/api-key.txt"

SYSTEMD_SERVICE:${PN} = "bkk-api-demo.service bkk-api-keygen.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    # Install main demo service
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/bkk-api-demo.service ${D}${systemd_system_unitdir}/bkk-api-demo.service
    
    # Install API key setup service
    install -m 0644 ${WORKDIR}/bkk-api-keygen.service ${D}${systemd_system_unitdir}/bkk-api-keygen.service
    
    # Install API key setup script
    install -d ${D}${libexecdir}/bkk-api
    install -m 0755 ${WORKDIR}/bkk-api-keygen.sh ${D}${libexecdir}/bkk-api/bkk-api-keygen.sh
    
    # Install API key file
    install -d ${D}${sysconfdir}/bkk-api
    install -m 0640 ${WORKDIR}/api-key.txt ${D}${sysconfdir}/bkk-api/api-key.txt
}
