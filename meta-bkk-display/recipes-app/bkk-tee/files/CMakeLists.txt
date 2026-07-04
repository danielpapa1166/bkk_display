cmake_minimum_required(VERSION 3.16)
project(bkk_key_ta_bundle LANGUAGES C)

add_subdirectory(src/client)
add_subdirectory(src/trusted_app)
add_subdirectory(test)
