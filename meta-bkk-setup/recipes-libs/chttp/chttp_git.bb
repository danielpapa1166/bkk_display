SUMMARY = "Minimal C HTTP server static library"
DESCRIPTION = "chttp provides a lightweight, route-based HTTP/1.1 server as a \
static library. Used by config-server for device setup."
HOMEPAGE = "https://github.com/danielpapa1166/chttp"
LICENSE = "CLOSED"

SRC_URI = "git://${TOPDIR}/../chttp;protocol=file;nobranch=1"
SRCREV = "d6fa8f41203e40616c4f933ec8eb662a55ae9ef0"

PV = "1.0+git${SRCPV}"

S = "${WORKDIR}/git"

inherit cmake

# Disable the example_server binary — only the library is needed.
EXTRA_OECMAKE += "-DCMAKE_SKIP_INSTALL_RULES=OFF"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 ${B}/libchttp.a ${D}${libdir}/libchttp.a

    install -d ${D}${includedir}
    install -m 0644 ${S}/include/chttp.h ${D}${includedir}/chttp.h
}

FILES:${PN}-staticdev += " \
    ${libdir}/libchttp.a \
    ${includedir}/chttp.h \
"

# Static library — nothing ships in the runtime package.
FILES:${PN} = ""
