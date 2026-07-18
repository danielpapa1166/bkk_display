set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

add_executable(bkk_screen_fb_playground
    main.c
)


install(TARGETS bkk_screen_fb_playground
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)