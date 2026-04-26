SUMMARY = "rbuflogd daemon and producer shared library"
DESCRIPTION = "Builds rbuflogd, installs librbuflogd_producer.so and enables rbuflogd.service"
HOMEPAGE = "https://github.com/danielpapa1166/rbuflogd"
LICENSE = "CLOSED"

FILESEXTRAPATHS:prepend := "${THISDIR}:"

SRC_URI = "git://${TOPDIR}/../rbuflogd;protocol=file;nobranch=1 \
           file://rbuflogd.service \
           "
SRCREV = "9793daba78f2e22200eecc3ce8527b780c26178e"

PV = "1.0+git${SRCPV}"

S = "${WORKDIR}/git"

inherit cmake pkgconfig systemd

do_install:append() {
    # Upstream CMake currently does not install the daemon target.
    install -d ${D}${bindir}
    install -m 0755 ${B}/rbuflogd ${D}${bindir}/rbuflogd

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/rbuflogd.service ${D}${systemd_system_unitdir}/rbuflogd.service
}

# Upstream ships an unversioned shared object (.so), keep it in the runtime package.
SOLIBS = ".so"
FILES_SOLIBSDEV = ""

FILES:${PN} += "${bindir}/rbuflogd"
FILES:${PN} += "${libdir}/librbuflogd_producer.so"
FILES:${PN} += "${includedir}/rbuflogd/*.h"
FILES:${PN} += "${systemd_system_unitdir}/rbuflogd.service"

INSANE_SKIP:${PN} += "dev-so"

SYSTEMD_SERVICE:${PN} = "rbuflogd.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"