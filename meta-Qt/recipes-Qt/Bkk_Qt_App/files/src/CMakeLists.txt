cmake_minimum_required(VERSION 3.16)
project(bkk_qt_app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ADS7846_CONTROLLER REQUIRED IMPORTED_TARGET ads7846-controller)

add_executable(bkk-qt-app
	main.cpp
	mainwindow.cpp
	mainwindow.hpp
	resources.qrc
	bkk_clock_update.cpp
	bkk_clock_update.hpp
	bkk_online_check.cpp
	bkk_online_check.hpp
	bkk_api_worker.cpp
	bkk_api_worker.hpp
	bkk_worker_thread.cpp
	bkk_worker_thread.hpp
	bkk_elapsed_timer.hpp
	bkk_logger.cpp
	bkk_logger.hpp
	bkk_touchscreen.cpp
	bkk_touchscreen.hpp
	bkk_touchscreen_feedback.cpp
	bkk_touchscreen_feedback.hpp
	bkk_arrival_table_handler.cpp
	bkk_arrival_table_handler.hpp
)

target_link_libraries(bkk-qt-app PRIVATE
	Qt5::Core
	Qt5::Widgets
	bkk_api_shared
	CURL::libcurl
	PkgConfig::ADS7846_CONTROLLER
)

install(TARGETS bkk-qt-app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
