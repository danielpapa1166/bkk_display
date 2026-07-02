cmake_minimum_required(VERSION 3.16)
project(bkk-screen-info-bar LANGUAGES C CXX)

include(GNUInstallDirs)

add_executable(bkk_screen_info_bar info_bar_main.cpp)


target_link_libraries(bkk_screen_info_bar
    PRIVATE
        bkk-screen-client
)


install(TARGETS bkk_screen_info_bar
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)