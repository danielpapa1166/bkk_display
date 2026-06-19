add_executable(bkk_key_client_test bkk_key_client_test.c)

target_include_directories(bkk_key_client_test
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../client
)


target_link_libraries(bkk_key_client_test
    PRIVATE
        bkk-key-client
)


target_compile_options(bkk_key_client_test PRIVATE -Wall -Wextra -pedantic)

install(TARGETS bkk_key_client_test
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)