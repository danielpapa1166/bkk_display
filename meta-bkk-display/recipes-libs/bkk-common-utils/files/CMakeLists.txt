cmake_minimum_required(VERSION 3.16)
project(bkk-screen-owner LANGUAGES C CXX)

add_subdirectory(timing)
add_subdirectory(online_status)
add_subdirectory(screen_backlight)
add_subdirectory(ipc_uds)
add_subdirectory(dbus)