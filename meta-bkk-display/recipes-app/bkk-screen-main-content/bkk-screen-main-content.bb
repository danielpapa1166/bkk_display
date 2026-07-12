LICENSE = "CLOSED"

inherit cmake pkgconfig

SRC_URI = "file://CMakeLists.txt        \
           file://content_main.cpp      \
           file://bkk_api_client.cpp    \
           file://bkk_api_client.hpp    \
           "

S = "${WORKDIR}"

DEPENDS = "bkk-screen-owner bkk-api rbuflogd bkk-tee"
RDEPENDS:${PN} += "bkk-screen-owner bkk-api bkk-api-client rbuflogd bkk-tee"

# Generate compile_commands.json for clangd tooling.
EXTRA_OECMAKE:append = " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"