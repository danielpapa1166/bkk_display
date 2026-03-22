cmake_minimum_required(VERSION 3.16)
project(ads7846_demo LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

pkg_check_modules(ADS7846 REQUIRED ads7846-controller)

add_executable(ads7846_demo ads7846_demo.c)

target_include_directories(ads7846_demo PRIVATE ${ADS7846_INCLUDE_DIRS})
target_link_libraries(ads7846_demo PRIVATE ${ADS7846_LIBRARIES})
target_link_directories(ads7846_demo PRIVATE ${ADS7846_LIBRARY_DIRS})

install(TARGETS ads7846_demo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
