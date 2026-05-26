# output binary: 
# /data/projects/bkk_display/build-rpi/tmp/work/cortexa7t2hf-neon-vfpv4-poky-linux-gnueabi/application-manager/0.1-r0/packages-split/application-manager/usr/bin/application_manager


cmake_minimum_required(VERSION 3.10)

project(application_manager C)

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# External source paths (used for local/native builds; ignored under Yocto
# where the sysroot-installed targets are found instead)
# ---------------------------------------------------------------------------
set(RBUFLOGD_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../rbuflogd"
    CACHE PATH "Path to the local rbuflogd source tree")

set(CJSON_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../cJSON"
    CACHE PATH "Path to the vendored cJSON source tree")

set(CHTTP_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../chttp"
    CACHE PATH "Path to the local chttp source tree")

# ---------------------------------------------------------------------------
# chttp
# ---------------------------------------------------------------------------
if(EXISTS "${CHTTP_SOURCE_DIR}/CMakeLists.txt")
    add_subdirectory("${CHTTP_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/chttp")
else()
    find_library(CHTTP_LIB REQUIRED NAMES chttp)
    find_path(CHTTP_INCLUDE_DIR REQUIRED NAMES chttp.h)
    add_library(chttp STATIC IMPORTED)
    set_target_properties(chttp PROPERTIES
        IMPORTED_LOCATION "${CHTTP_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${CHTTP_INCLUDE_DIR}")
endif()

# ---------------------------------------------------------------------------
# cJSON
# ---------------------------------------------------------------------------
if(EXISTS "${CJSON_SOURCE_DIR}/CMakeLists.txt")
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
    set(ENABLE_CJSON_UTILS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CJSON_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/cJSON")
    set(CJSON_INCLUDE_DIR "${CJSON_SOURCE_DIR}")
else()
    find_library(CJSON_LIB REQUIRED NAMES cjson)
    find_path(CJSON_INCLUDE_DIR REQUIRED NAMES cJSON.h PATH_SUFFIXES cjson cJSON)
        add_library(cjson SHARED IMPORTED)
        set_target_properties(cjson PROPERTIES
            IMPORTED_LOCATION "${CJSON_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${CJSON_INCLUDE_DIR}")
endif()

# ---------------------------------------------------------------------------
# rbuflogd producer
# ---------------------------------------------------------------------------
if(EXISTS "${RBUFLOGD_SOURCE_DIR}/CMakeLists.txt")
    add_subdirectory("${RBUFLOGD_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/rbuflogd")
else()
    find_library(RBUFLOGD_PRODUCER_LIB REQUIRED NAMES rbuflogd_producer)
    find_path(RBUFLOGD_INCLUDE_DIR REQUIRED NAMES rbuflogd/producer.h)
    add_library(rbuflogd_producer SHARED IMPORTED)
    set_target_properties(rbuflogd_producer PROPERTIES
        IMPORTED_LOCATION "${RBUFLOGD_PRODUCER_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${RBUFLOGD_INCLUDE_DIR}")
endif()

# ---------------------------------------------------------------------------
# application_manager executable
# ---------------------------------------------------------------------------
add_executable(application_manager
    am_main.c
    am_config_parser.c
    am_launcher.c
    am_supervisor.c
    am_boot_mode.c
    am_http_server.c
)

target_include_directories(application_manager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CJSON_INCLUDE_DIR}
)

target_link_libraries(application_manager PRIVATE
    cjson
    chttp
    rbuflogd_producer
)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS application_manager
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---------------------------------------------------------------------------
# Sub-components
# ---------------------------------------------------------------------------
add_subdirectory(wpa_helper)
add_subdirectory(logger_check)
add_subdirectory(networkd_check)

if(TARGET cjson)
    get_target_property(CJSON_TARGET_IMPORTED cjson IMPORTED)
    if(NOT CJSON_TARGET_IMPORTED)
        install(TARGETS cjson
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()
endif()
