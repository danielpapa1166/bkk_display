cmake_minimum_required(VERSION 3.10)

project(c_http_server C)

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(RBUFLOGD_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../rbuflogd"
    CACHE PATH "Path to the local rbuflogd source tree")

add_executable(c-http-server
    http_server_main.c
    http_server_client_handler.c
    http_server_resource_handler.c
    http_server_post_handler.c
)

target_include_directories(c-http-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(EXISTS "${RBUFLOGD_SOURCE_DIR}/CMakeLists.txt")
    add_subdirectory("${RBUFLOGD_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/rbuflogd")
    target_link_libraries(c-http-server PRIVATE rbuflogd_producer)
else()
    find_library(RBUFLOGD_PRODUCER_LIB REQUIRED NAMES rbuflogd_producer)
    find_path(RBUFLOGD_INCLUDE_DIR REQUIRED NAMES rbuflogd/producer.h)

    target_include_directories(c-http-server PRIVATE ${RBUFLOGD_INCLUDE_DIR})
    target_link_libraries(c-http-server PRIVATE ${RBUFLOGD_PRODUCER_LIB})
endif()

install(TARGETS c-http-server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)