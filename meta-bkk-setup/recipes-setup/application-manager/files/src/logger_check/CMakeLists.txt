add_executable(logger_check
    logger_check_main.c
)

target_link_libraries(logger_check PRIVATE rbuflogd_producer)

install(TARGETS logger_check
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
