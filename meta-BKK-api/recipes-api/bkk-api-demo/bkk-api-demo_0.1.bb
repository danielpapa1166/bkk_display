SUMMARY = "Minimal BKK API demo application"
DESCRIPTION = "Small C++ app that uses BkkApi and calls a member function"
HOMEPAGE = "https://example.local/bkk-api-demo"
LICENSE = "CLOSED"

SRC_URI = " \
    file://CMakeLists.txt \
    file://main.cpp \
    file://bkk-api-demo.service \
"

S = "${WORKDIR}"

inherit cmake pkgconfig systemd

DEPENDS = "bkk-api"
RDEPENDS:${PN} += "bkk-api bkk-api-keyenv"

FILES:${PN} += "${bindir}/bkk-api-demo"
FILES:${PN} += "${systemd_system_unitdir}/bkk-api-demo.service"


# Enable compile_commands.json generation for clangd tooling.
EXTRA_OECMAKE:append = " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

SYSTEMD_SERVICE:${PN} = "bkk-api-demo.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    # Install main demo service
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/bkk-api-demo.service ${D}${systemd_system_unitdir}/bkk-api-demo.service
}
