add_executable(networkd_check
    networkd_check_main.c
)

target_include_directories(networkd_check PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(networkd_check PRIVATE
    rbuflogd_producer
    systemd
)

install(TARGETS networkd_check
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
