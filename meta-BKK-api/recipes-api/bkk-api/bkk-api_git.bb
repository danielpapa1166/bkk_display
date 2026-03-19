SUMMARY = "BKK API C++ shared library"
DESCRIPTION = "Builds libbkk_api_shared.so and installs public C++ headers"
HOMEPAGE = "https://github.com/danielpapa1166/bkk_api"
LICENSE = "CLOSED"

SRC_URI = "git://github.com/danielpapa1166/bkk_api.git;protocol=ssh;branch=main"
SRCREV = "104d6c6fb8037672cb3b7ae0412302cf19e1a746"

PV = "1.0+git${SRCPV}"

S = "${WORKDIR}/git/cpp"

inherit cmake pkgconfig

DEPENDS = "curl nlohmann-json"

# Prevent CMake from trying to fetch dependencies from the network in Yocto tasks.
EXTRA_OECMAKE += "-DFETCHCONTENT_FULLY_DISCONNECTED=ON"

do_install() {
    install -d ${D}${libdir}
    install -m 0755 ${B}/libbkk_api_shared.so ${D}${libdir}/

    install -d ${D}${includedir}/bkk_api
    install -m 0644 ${S}/include/*.hpp ${D}${includedir}/bkk_api/
}

# Upstream ships a real, unversioned shared library (.so), not a symlinked soname.
# Keep it out of -dev so package_qa does not classify it as [dev-elf].
SOLIBS = ".so"
FILES_SOLIBSDEV = ""

FILES:${PN} = "${libdir}/libbkk_api_shared.so"
FILES:${PN}-dev += "${includedir}/bkk_api/*.hpp"

# Upstream produces an unversioned shared object; keep it in the runtime package.
INSANE_SKIP:${PN} += "dev-so"
