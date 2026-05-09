SUMMARY = "BKK public transport API – UDS server daemon and client library"
DESCRIPTION = "Builds bkk_uds_server (daemon), libbkk_uds_client.so (shared library), \
               and bkk_uds_test (test tool)."
HOMEPAGE = "https://github.com/danielpapa1166/bkk_api"
LICENSE = "CLOSED"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI = "git://${TOPDIR}/../bkk_api;protocol=file;nobranch=1;name=bkk_api \
           git://github.com/DaveGamble/cJSON.git;protocol=https;nobranch=1;name=cjson;destsuffix=git/submodules/cJSON \
           git://github.com/danielpapa1166/rbuflogd.git;protocol=https;nobranch=1;name=rbuflogd;destsuffix=git/submodules/rbuflogd \
           file://bkk-uds-server.service \
           "

SRCREV_bkk_api  = "eaf8e1bf359c2be3b533a9ea58bf393e28df37e7"
SRCREV_cjson    = "fb16e5cf358798aabb049655975cde8427101056"
SRCREV_rbuflogd = "83748e55788b6d8615df4ae5388d360db0673e44"
SRCREV_FORMAT   = "bkk_api_cjson_rbuflogd"

PV = "1.0+git${SRCPV}"

S = "${WORKDIR}/git"

inherit cmake pkgconfig systemd

DEPENDS = "curl"

# rbuflogd is built inline from the submodule; runtime .so comes from the rbuflogd package.
RDEPENDS:${PN} = "rbuflogd"

EXTRA_OECMAKE = "-DBKK_API_VERBOSE_ON=OFF"

do_install() {
    install -d ${D}${bindir}
    install -d ${D}${libdir}
    install -d ${D}${includedir}/bkk_uds
    install -d ${D}${systemd_system_unitdir}

    install -m 0755 ${B}/bin/bkk_uds_server        ${D}${bindir}/bkk_uds_server
    install -m 0755 ${B}/bin/bkk_uds_test           ${D}${bindir}/bkk_uds_test
    install -m 0755 ${B}/lib/libbkk_uds_client.so   ${D}${libdir}/libbkk_uds_client.so

    install -m 0644 ${S}/bkk_uds/bkk_uds_client.h   ${D}${includedir}/bkk_uds/bkk_uds_client.h
    install -m 0644 ${S}/bkk_uds/bkk_uds_protocol.h ${D}${includedir}/bkk_uds/bkk_uds_protocol.h
    install -m 0644 ${S}/bkk_uds/bkk_api_arrival.h  ${D}${includedir}/bkk_uds/bkk_api_arrival.h
    install -m 0644 ${S}/bkk_uds/bkk_stop_list.h    ${D}${includedir}/bkk_uds/bkk_stop_list.h
    install -m 0644 ${S}/bkk_uds/bkk_stop_utils.h   ${D}${includedir}/bkk_uds/bkk_stop_utils.h

    install -m 0644 ${WORKDIR}/bkk-uds-server.service \
        ${D}${systemd_system_unitdir}/bkk-uds-server.service
}

# Allow unversioned .so to stay in the runtime package (no SONAME versioning).
SOLIBS = ".so"
FILES_SOLIBSDEV = ""

PACKAGES =+ "${PN}-client ${PN}-test"

FILES:${PN} = " \
    ${bindir}/bkk_uds_server \
    ${systemd_system_unitdir}/bkk-uds-server.service \
"

FILES:${PN}-client = "${libdir}/libbkk_uds_client.so"

FILES:${PN}-test = "${bindir}/bkk_uds_test"

RDEPENDS:${PN}-test = "${PN}"

INSANE_SKIP:${PN} += "dev-so"

SYSTEMD_SERVICE:${PN} = "bkk-uds-server.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"
