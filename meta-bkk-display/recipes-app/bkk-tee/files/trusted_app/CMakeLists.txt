cmake_minimum_required(VERSION 3.16)
project(bkk_tee LANGUAGES C)

include(GNUInstallDirs)

set(BKK_TEE_UUID "8f6f7b8a-21a4-4de8-9b8d-7c0b96648c19" CACHE STRING "TA UUID used for output .ta filename")
set(OPTEE_TA_DEV_KIT_DIR "" CACHE PATH "Path to OP-TEE export-user_ta directory")
set(TA_CROSS_COMPILE "" CACHE STRING "Cross-compile prefix for TA toolchain")
set(TA_SYSROOT "" CACHE PATH "Target sysroot used to locate libgcc and runtime libs")

if(NOT OPTEE_TA_DEV_KIT_DIR)
	message(FATAL_ERROR "OPTEE_TA_DEV_KIT_DIR must be set")
endif()

if(NOT TA_SYSROOT)
	message(FATAL_ERROR "TA_SYSROOT must be set")
endif()

set(TA_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/ta-build)
set(TA_OUTPUT ${TA_BUILD_DIR}/${BKK_TEE_UUID}.ta)

add_custom_command(
	OUTPUT ${TA_OUTPUT}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${TA_BUILD_DIR}
	COMMAND ${CMAKE_COMMAND} -E env
			TA_DEV_KIT_DIR=${OPTEE_TA_DEV_KIT_DIR}
			BINARY=${BKK_TEE_UUID}
			O=${TA_BUILD_DIR}
			CROSS_COMPILE=${TA_CROSS_COMPILE}
			LIBGCC_LOCATE_CFLAGS=--sysroot=${TA_SYSROOT}
			CFLAGS32=--sysroot=${TA_SYSROOT}
			CFLAGS64=--sysroot=${TA_SYSROOT}
			make -C ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/bkk_tee_ta.c
		${CMAKE_CURRENT_SOURCE_DIR}/Makefile
		${CMAKE_CURRENT_SOURCE_DIR}/sub.mk
		${CMAKE_CURRENT_SOURCE_DIR}/user_ta_header_defines.h
	VERBATIM
)

add_custom_target(bkk-tee-ta ALL DEPENDS ${TA_OUTPUT})

install(FILES ${TA_OUTPUT} DESTINATION ${CMAKE_INSTALL_LIBDIR}/optee_armtz)
