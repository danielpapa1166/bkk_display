add_library(bkk-screen-client SHARED bkk_screen_client.cpp)

target_include_directories(bkk-screen-client 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/
)