cmake_minimum_required(VERSION 3.16)
project(dbus_test LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

add_subdirectory(src)
add_subdirectory(test)