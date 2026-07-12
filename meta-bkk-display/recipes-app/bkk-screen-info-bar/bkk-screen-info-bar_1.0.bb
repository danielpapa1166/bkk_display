LICENSE = "CLOSED"

inherit cmake

SRC_URI = "file://CMakeLists.txt        \
           file://info_bar_main.cpp     \
           file://clock_update.cpp      \
           file://clock_update.hpp      \
           file://online_check.cpp      \
           file://online_check.hpp      \
           "

S = "${WORKDIR}"

DEPENDS = "bkk-screen-owner"
RDEPENDS:${PN} += " bkk-screen-owner"

# Generate compile_commands.json for clangd tooling.
EXTRA_OECMAKE:append = " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"