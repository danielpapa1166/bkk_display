cmake_minimum_required(VERSION 3.16)
project(bkk_web_setup_helper LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

add_executable(bkk-web-setup-helper
    helper_main.cpp
    helper_mainwindow.cpp helper_mainwindow.hpp
    resources.qrc
)

target_link_libraries(bkk-web-setup-helper PRIVATE
    Qt5::Core Qt5::Widgets
)

install(TARGETS bkk-web-setup-helper RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
