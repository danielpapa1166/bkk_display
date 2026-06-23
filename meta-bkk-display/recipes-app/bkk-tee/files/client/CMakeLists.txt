cmake_minimum_required(VERSION 3.16)
project(bkk_tee_client LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

find_path(TEEC_INCLUDE_DIR NAMES tee_client_api.h)
find_library(TEEC_LIBRARY NAMES teec)

if(NOT TEEC_INCLUDE_DIR)
	message(FATAL_ERROR "tee_client_api.h not found")
endif()

if(NOT TEEC_LIBRARY)
	message(FATAL_ERROR "libteec not found")
endif()

add_library(bkk-tee-client SHARED bkk_tee_client.c)

set_target_properties(bkk-tee-client PROPERTIES
	VERSION 1.0.0
	SOVERSION 1
)

target_include_directories(bkk-tee-client
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../common
		${TEEC_INCLUDE_DIR}
)

target_compile_options(bkk-tee-client PRIVATE -Wall -Wextra -pedantic -g -O0)


# add rbuflogd lib: 

option(RBUFLOGD_USE_SYSROOT "Use rbuflogd from sysroot (find_library) instead of submodule" OFF)

if(NOT RBUFLOGD_USE_SYSROOT)
    file(REAL_PATH
        "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../submodules/rbuflogd"
        RBUFLOGD_SOURCE_DIR)
    message(STATUS "Using RBUFLOGD_SOURCE_DIR=${RBUFLOGD_SOURCE_DIR}")
endif()

if(NOT RBUFLOGD_USE_SYSROOT AND EXISTS "${RBUFLOGD_SOURCE_DIR}/CMakeLists.txt")
    add_subdirectory("${RBUFLOGD_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/rbuflogd")
else()
    find_library(RBUFLOGD_PRODUCER_LIB REQUIRED NAMES rbuflogd_producer)
    find_path(RBUFLOGD_INCLUDE_DIR REQUIRED NAMES rbuflogd/producer.h)
    add_library(rbuflogd_producer SHARED IMPORTED)
    set_target_properties(rbuflogd_producer PROPERTIES
        IMPORTED_LOCATION "${RBUFLOGD_PRODUCER_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${RBUFLOGD_INCLUDE_DIR}")
endif()

target_link_libraries(bkk-tee-client PRIVATE 
	${TEEC_LIBRARY}
	rbuflogd_producer)

install(TARGETS bkk-tee-client
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES bkk_tee_client.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bkk-tee-client
)
