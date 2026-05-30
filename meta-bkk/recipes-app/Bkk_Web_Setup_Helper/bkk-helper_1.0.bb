SUMMARY = "BKK Display Web Setup Helper Application"
LICENSE = "CLOSED"

inherit cmake_qt5 pkgconfig systemd externalsrc

# Source lives in submodules/bkk_web_setup at the project root.
EXTERNALSRC = "${TOPDIR}/../submodules/bkk_web_setup"
EXTERNALSRC_BUILD = "${WORKDIR}/build"

SRC_URI = "file://icon/ \
           file://bkk-web-setup-helper.service \
           "

DEPENDS = "qtbase rbuflogd"
RDEPENDS:${PN} += "rbuflogd"


# Generate compile_commands.json for clangd tooling.
EXTRA_OECMAKE:append = " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

SYSTEMD_SERVICE:${PN} = "bkk-web-setup-helper.service"
SYSTEMD_AUTO_ENABLE:${PN} = "disable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/bkk-web-setup-helper.service ${D}${systemd_system_unitdir}/bkk-web-setup-helper.service
}