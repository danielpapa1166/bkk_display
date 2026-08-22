SUMMARY = "Version 3-M QR code generator"
DESCRIPTION = "C++ library for generating Version 3-M QR code matrices"
HOMEPAGE = "https://github.com/danielpapa1166/qr_code_gen"
LICENSE = "CLOSED"

SRC_URI = "git://${TOPDIR}/../submodules/qr_code_gen;protocol=file;nobranch=1"
SRCREV = "a969c0907419c231665e057b0aed23dfc35763a9"

PV = "0.1.0+git${SRCPV}"

S = "${WORKDIR}/git"

inherit cmake

EXTRA_OECMAKE += "-DQR_CODE_GEN_BUILD_SANITY_APP=OFF"