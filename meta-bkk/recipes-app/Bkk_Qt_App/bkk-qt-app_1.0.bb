SUMMARY = "BKK Dislpay Main Application"
LICENSE = "CLOSED"

inherit cmake_qt5 pkgconfig systemd externalsrc

# Source lives in submodules/bkk_qt_app at the project root.
# EXTERNALSRC skips Yocto fetch/unpack; edits to the source dir are picked up
# on the next bitbake run.
EXTERNALSRC = "${TOPDIR}/../submodules/bkk_qt_app"
EXTERNALSRC_BUILD = "${WORKDIR}/build"

SRC_URI = "file://icon/ \
           file://bkk-qt-app.service \
           "

DEPENDS = "qtbase bkk-api ads7846-controller rbuflogd cjson"
RDEPENDS:${PN} += "bkk-api bkk-api-client bkk-api-keyenv rbuflogd"

# Generate compile_commands.json for clangd tooling.
EXTRA_OECMAKE:append = " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

SYSTEMD_SERVICE:${PN} = "bkk-qt-app.service"
SYSTEMD_AUTO_ENABLE:${PN} = "disable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/bkk-qt-app.service ${D}${systemd_system_unitdir}/bkk-qt-app.service
}