project(bkk_screen_backlight LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(GNUInstallDirs)

add_library(
    bkk_screen_backlight SHARED 
    src/bkk_screen_backlight.c    
)

target_include_directories(
    bkk_screen_backlight PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(
    bkk_screen_backlight PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    PUBLIC_HEADER include/bkk_screen_backlight.h
)

install(TARGETS bkk_screen_backlight
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bkk_utils
)