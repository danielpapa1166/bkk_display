cmake_minimum_required(VERSION 3.16)
project(bkk-screen-owner LANGUAGES C CXX)

include(GNUInstallDirs)

add_subdirectory(src)

install(FILES include/network_manager_pub.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)