cmake_minimum_required(VERSION 3.16)
project(bkk-screen-main-content LANGUAGES C CXX)

include(GNUInstallDirs)

find_package(CURL REQUIRED)

add_executable(bkk_screen_main_content
    content_main.cpp)


target_link_libraries(bkk_screen_main_content
    PRIVATE
        bkk-screen-client
        rbuflogd_producer
)


install(TARGETS bkk_screen_main_content
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)