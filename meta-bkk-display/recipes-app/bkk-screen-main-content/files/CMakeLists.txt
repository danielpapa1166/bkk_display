cmake_minimum_required(VERSION 3.16)
project(bkk-screen-main-content LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)

find_library(BKK_TEE_LIB REQUIRED NAMES bkk-tee-client)
add_library(bkk-tee SHARED IMPORTED)

add_executable(bkk_screen_main_content
    content_main.cpp
    bkk_api_client.cpp
    screen_context.cpp 
    api_context.cpp
    content_state_machine.cpp
)


target_link_libraries(bkk_screen_main_content
    PRIVATE
        bkk-screen-client
        rbuflogd_producer
        bkk_uds_client
        PkgConfig::CJSON
        bkk-tee-client
        bkk_utils_timing
        bkk-utils-dbus
)


install(TARGETS bkk_screen_main_content
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)