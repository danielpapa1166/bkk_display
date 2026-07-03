cmake_minimum_required(VERSION 3.16)
project(bkk-screen-info-bar LANGUAGES C CXX)

include(GNUInstallDirs)

find_package(CURL REQUIRED)

add_executable(bkk_screen_info_bar 
    info_bar_main.cpp
    clock_update.cpp
    online_check.cpp)


target_link_libraries(bkk_screen_info_bar
    PRIVATE
        bkk-screen-client
        CURL::libcurl
)


install(TARGETS bkk_screen_info_bar
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)