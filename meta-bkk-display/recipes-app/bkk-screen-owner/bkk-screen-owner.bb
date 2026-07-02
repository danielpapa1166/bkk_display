SUMMARY = "BKK Dislpay Main Application"
LICENSE = "CLOSED"

inherit cmake_qt5 pkgconfig systemd

SRC_URI = "file://CMakeLists.txt        \
           file://client/               \
           file://common/               \
           file://include/              \
           file://owner/                \
           file://icon/                 \
           "

S = "${WORKDIR}/src"

DEPENDS = "qtbase bkk-api ads7846-controller rbuflogd cjson bkk-tee"
RDEPENDS:${PN} += "bkk-api bkk-api-client bkk-api-keyenv rbuflogd bkk-tee"

# Generate compile_commands.json for clangd tooling.
EXTRA_OECMAKE:append = " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
