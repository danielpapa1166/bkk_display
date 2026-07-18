project(bkk_utils_online_status LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCURL REQUIRED IMPORTED_TARGET libcurl)


add_library(
    bkk_utils_online_status SHARED 
    src/bkk_utils_online_status.c
)

target_include_directories(bkk_utils_online_status PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(bkk_utils_online_status PRIVATE PkgConfig::LIBCURL)



set_target_properties(
    bkk_utils_online_status PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    PUBLIC_HEADER include/bkk_utils_online_status.h
)


install(TARGETS bkk_utils_online_status
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bkk_utils
)