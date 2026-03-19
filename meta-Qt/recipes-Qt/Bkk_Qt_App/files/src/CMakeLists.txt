cmake_minimum_required(VERSION 3.16)
project(bkk_qt_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(bkk-qt-app main.cpp)

target_link_libraries(bkk-qt-app PRIVATE Qt5::Widgets)

install(TARGETS bkk-qt-app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
