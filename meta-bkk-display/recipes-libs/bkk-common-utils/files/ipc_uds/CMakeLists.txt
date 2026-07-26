project(bkk_utils_ipc_uds LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(GNUInstallDirs)


add_library(
    bkk_utils_ipc_uds SHARED 
    src/bkk_ipc_uds.c
)

target_include_directories(bkk_utils_ipc_uds PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


set_target_properties(
    bkk_utils_ipc_uds PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    PUBLIC_HEADER 
        "include/bkk_ipc_uds_server.h;include/bkk_ipc_uds_client.h;include/bkk_ipc_uds_common_defs.h"

)

install(TARGETS bkk_utils_ipc_uds
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bkk_utils
)