cmake_minimum_required(VERSION 3.10)

project(c_http_server C)

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# External source paths
# ---------------------------------------------------------------------------
set(RBUFLOGD_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../rbuflogd"
    CACHE PATH "Path to the local rbuflogd source tree")

set(CJSON_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../cJSON"
    CACHE PATH "Path to the vendored cJSON source tree")

# ---------------------------------------------------------------------------
# cJSON shared library
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
# rbuflogd
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
# c-http-server executable
# ---------------------------------------------------------------------------
add_executable(c-http-server
    http_server_main.c
    http_server_client_handler.c
    http_server_resource_handler.c
    http_server_post_handler.c
    http_server_user_action_handler.c
)

target_include_directories(c-http-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CJSON_INCLUDE_DIR}
)

target_link_libraries(c-http-server PRIVATE
    cjson
    rbuflogd_producer
)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS c-http-server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(TARGET cjson)
    get_target_property(CJSON_TARGET_IMPORTED cjson IMPORTED)
    if(NOT CJSON_TARGET_IMPORTED)
        install(TARGETS cjson
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()
endif()