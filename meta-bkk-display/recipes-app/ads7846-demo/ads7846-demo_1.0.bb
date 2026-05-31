SUMMARY = "ADS7846 touch controller demo application"
DESCRIPTION = "Simple CLI app that prints ADS7846 touch events using the ads7846-controller library"
LICENSE = "CLOSED"

SRC_URI = "file://CMakeLists.txt \
           file://ads7846_demo.c \
           "

S = "${WORKDIR}"

DEPENDS = "ads7846-controller"

inherit cmake pkgconfig
