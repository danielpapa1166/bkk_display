SUMMARY = "HTTP Server"
DESCRIPTION = "Minimal Yocto recipe that builds a C app from files/src and installs static web assets"
LICENSE = "CLOSED"

inherit cmake

SRC_URI = "file://src/CMakeLists.txt \
           file://src/http_server_main.c \
           file://src/http_server_client_handler.c \
           file://src/http_server_client_handler.h \
           file://src/http_server_post_handler.c \
           file://src/http_server_post_handler.h \
           file://src/http_server_resource_handler.c \
           file://src/http_server_resource_handler.h \
           file://src/http_server_utils.h \
           file://www/index.html \
           file://www/styles.css \
           file://www/app.js \
"

DEPENDS = "rbuflogd cjson"
RDEPENDS:${PN} += " rbuflogd cjson"

S = "${WORKDIR}/src"

# c-http-server-lib is built as an unversioned .so runtime library.
# Treat .so as a runtime lib so it is shipped in ${PN} instead of -dev.
SOLIBS = ".so"
FILES_SOLIBSDEV = ""

FILES:${PN} += " \
    ${bindir}/c-http-server \
    ${libdir}/libc-http-server-lib.so \
"

do_install:append() {
    install -d ${D}${datadir}/c-http-server/www
    install -m 0644 ${WORKDIR}/www/index.html ${D}${datadir}/c-http-server/www/index.html
    install -m 0644 ${WORKDIR}/www/styles.css ${D}${datadir}/c-http-server/www/styles.css
    install -m 0644 ${WORKDIR}/www/app.js ${D}${datadir}/c-http-server/www/app.js
}
