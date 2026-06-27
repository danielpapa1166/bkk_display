add_executable(bkk_tee_client_test bkk_tee_client_test.c)

target_include_directories(bkk_tee_client_test
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../client
)


target_link_libraries(bkk_tee_client_test
    PRIVATE
        bkk-tee-client
)


target_compile_options(bkk_tee_client_test PRIVATE -Wall -Wextra -pedantic -g -O0)

install(TARGETS bkk_tee_client_test
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)