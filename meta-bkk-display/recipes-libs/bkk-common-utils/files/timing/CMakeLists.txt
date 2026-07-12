project(bkk_utils_timing LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(GNUInstallDirs)
find_package(Threads REQUIRED)

add_library(
    bkk_utils_timing SHARED 
    src/bkk_utils_timing.c    
)

target_include_directories(bkk_utils_timing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(bkk_utils_timing PRIVATE Threads::Threads)

set_target_properties(
    bkk_utils_timing PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    PUBLIC_HEADER include/bkk_utils_timing.h
)

install(TARGETS bkk_utils_timing
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bkk_utils
)