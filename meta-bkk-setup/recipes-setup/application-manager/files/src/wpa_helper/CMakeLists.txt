add_executable(wpa_helper
    wpa_helper_main.c
)

install(TARGETS wpa_helper
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
