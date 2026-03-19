cmake_minimum_required(VERSION 3.16)
project(bkk_api_demo LANGUAGES CXX)

include(GNUInstallDirs)

add_executable(bkk-api-demo main.cpp)
target_compile_features(bkk-api-demo PRIVATE cxx_std_17)

target_link_libraries(bkk-api-demo PRIVATE bkk_api_shared)

install(TARGETS bkk-api-demo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
