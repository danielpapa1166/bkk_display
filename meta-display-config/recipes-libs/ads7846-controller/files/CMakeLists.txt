cmake_minimum_required(VERSION 3.16)
project(ads7846_controller LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON),

include(GNUInstallDirs)
find_package(Threads REQUIRED)

add_library(ads7846_controller SHARED
    ads7846_controller.c
)

set_target_properties(ads7846_controller PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    PUBLIC_HEADER ads7846_controller.h
)

target_include_directories(ads7846_controller PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ads7846_controller>
)

target_link_libraries(ads7846_controller PUBLIC
    Threads::Threads
)

install(TARGETS ads7846_controller
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ads7846_controller
)

configure_file(ads7846-controller.pc.in ads7846-controller.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ads7846-controller.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
