SUMMARY = "BKK Dislpay Main Application"
LICENSE = "CLOSED"

inherit cmake_qt5 pkgconfig systemd

SRC_URI = "file://src/ \
           file://icon/ \
           file://bkk-qt-app.service \
           "
S = "${WORKDIR}/src"

DEPENDS = "qtbase bkk-api ads7846-controller rbuflogd"
RDEPENDS:${PN} += "bkk-api bkk-api-keyenv rbuflogd"

# Generate compile_commands.json for clangd tooling.
EXTRA_OECMAKE:append = " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

SYSTEMD_SERVICE:${PN} = "bkk-qt-app.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/bkk-qt-app.service ${D}${systemd_system_unitdir}/bkk-qt-app.service
}