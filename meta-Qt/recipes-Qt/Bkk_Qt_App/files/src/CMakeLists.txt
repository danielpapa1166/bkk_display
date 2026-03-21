cmake_minimum_required(VERSION 3.16)
project(bkk_qt_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(CURL REQUIRED)

add_executable(bkk-qt-app
	main.cpp
	mainwindow.cpp
	mainwindow.hpp
	resources.qrc
	bkk_clock_update.cpp
	bkk_clock_update.hpp
	bkk_online_check.cpp
	bkk_online_check.hpp
	bkk_api_wrapper.cpp
	bkk_api_wrapper.hpp
	bkk_worker_thread.cpp
	bkk_worker_thread.hpp
)

target_link_libraries(bkk-qt-app PRIVATE
	Qt5::Core
	Qt5::Widgets
	bkk_api_shared
	CURL::libcurl
)

install(TARGETS bkk-qt-app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
