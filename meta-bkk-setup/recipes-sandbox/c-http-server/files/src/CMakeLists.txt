cmake_minimum_required(VERSION 3.10)

project(c_http_server C)

include(GNUInstallDirs)

add_executable(c-http-server
    http_server_main.c
    http_server_client_handler.c
    http_server_resource_handler.c
)

target_include_directories(c-http-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS c-http-server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)