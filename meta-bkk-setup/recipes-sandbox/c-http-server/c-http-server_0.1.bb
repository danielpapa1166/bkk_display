SUMMARY = "HTTP Server"
DESCRIPTION = "Minimal Yocto recipe that builds a C app from files/ and installs it on target"
LICENSE = "CLOSED"

inherit cmake

SRC_URI = "file://CMakeLists.txt \
           file://http_server_main.c \
           file://http_server_client_handler.c \
           file://http_server_client_handler.h \
"

S = "${WORKDIR}"
