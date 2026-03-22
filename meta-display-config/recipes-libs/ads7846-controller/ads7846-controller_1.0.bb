SUMMARY = "ADS7846 userspace shared library"
DESCRIPTION = "Shared library for ADS7846 SPI access and IRQ callback handling"
LICENSE = "CLOSED"

SRC_URI = "file://CMakeLists.txt \
           file://ads7846_controller.c \
           file://ads7846_controller.h \
           file://ads7846-controller.pc.in \
           "

S = "${WORKDIR}"

inherit cmake pkgconfig
