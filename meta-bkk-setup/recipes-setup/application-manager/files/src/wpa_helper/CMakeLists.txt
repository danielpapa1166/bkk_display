add_executable(wpa_helper
    wpa_helper_main.c
)

target_link_libraries(wpa_helper PRIVATE rbuflogd_producer)

install(TARGETS wpa_helper
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
