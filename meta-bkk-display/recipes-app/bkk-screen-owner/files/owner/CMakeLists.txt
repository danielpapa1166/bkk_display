cmake_minimum_required(VERSION 3.16)
project(bkk_qt_app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(PkgConfig REQUIRED)

pkg_check_modules(ADS7846_CONTROLLER REQUIRED IMPORTED_TARGET ads7846-controller)

add_executable(bkk-screen-owner
	main.cpp
	screen.cpp
	info_bar_req_handler.cpp
	component_req_handler.cpp
	table_req_handler.cpp
	status_screen_req_handler.cpp
	user_touch_handler.cpp
	resources.qrc
)


target_compile_options(bkk-screen-owner 
	PRIVATE -Wall -Wextra -pedantic -g -O0)


target_include_directories(bkk-screen-owner
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common
		${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(bkk-screen-owner PRIVATE
	Qt5::Core
	Qt5::Widgets
	bkk_uds_client
	rbuflogd_producer
	PkgConfig::ADS7846_CONTROLLER
	bkk_utils_timing
	bkk_screen_backlight
	bkk_utils_ipc_uds
)

install(TARGETS bkk-screen-owner RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
