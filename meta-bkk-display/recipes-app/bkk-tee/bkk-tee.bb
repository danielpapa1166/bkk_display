SUMMARY = "BKK OP-TEE trusted app and client shared library"
DESCRIPTION = "Builds BKK key trusted app (.ta) and libbkk-key-client.so"
LICENSE = "CLOSED"

inherit cmake pkgconfig

SRC_URI = "file://CMakeLists.txt \
		   file://client/CMakeLists.txt \
		   file://client/bkk_key_client.c \
		   file://client/bkk_key_client.h \
		   file://trusted_app/CMakeLists.txt \
		   file://trusted_app/Makefile \
		   file://trusted_app/sub.mk \
		   file://trusted_app/user_ta_header_defines.h \
		   file://trusted_app/bkk_tee_ta.c \
		   file://test/bkk_key_client_test.c \
		   file://test/CMakeLists.txt \
"

S = "${WORKDIR}"

DEPENDS = "optee-client optee-os-tadevkit rbuflogd"
RDEPENDS:${PN} += " rbuflogd"



EXTRA_OECMAKE += " \
	-DOPTEE_TA_DEV_KIT_DIR=${STAGING_INCDIR}/optee/export-user_ta \
	-DTA_CROSS_COMPILE=${HOST_PREFIX} \
	-DTA_SYSROOT=${STAGING_DIR_HOST} \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DRBUFLOGD_USE_SYSROOT=ON \
"

FILES:${PN} += " \
	${libdir}/libbkk-key-client.so.* \
	${base_libdir}/optee_armtz/*.ta \
"

do_install:append() {
	install -d ${D}${base_libdir}/optee_armtz
	mv ${D}${libdir}/optee_armtz/*.ta ${D}${base_libdir}/optee_armtz/
	rmdir --ignore-fail-on-non-empty ${D}${libdir}/optee_armtz
}

FILES:${PN}-dev += " \
	${includedir}/bkk-key-client/bkk_key_client.h \
	${libdir}/libbkk-key-client.so \
"