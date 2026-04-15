SUMMARY = "HTTP Server"
DESCRIPTION = "Minimal Yocto recipe that builds a C app from files/ and installs it on target"
LICENSE = "CLOSED"

SRC_URI = "file://http_server_main.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} http_server_main.c -o c-http-server
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 c-http-server ${D}${bindir}/c-http-server
}
