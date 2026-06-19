cmake_minimum_required(VERSION 3.16)
project(bkk_key_client LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

find_path(TEEC_INCLUDE_DIR NAMES tee_client_api.h)
find_library(TEEC_LIBRARY NAMES teec)

if(NOT TEEC_INCLUDE_DIR)
	message(FATAL_ERROR "tee_client_api.h not found")
endif()

if(NOT TEEC_LIBRARY)
	message(FATAL_ERROR "libteec not found")
endif()

add_library(bkk-key-client SHARED bkk_key_client.c)

set_target_properties(bkk-key-client PROPERTIES
	VERSION 1.0.0
	SOVERSION 1
)

target_include_directories(bkk-key-client
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${TEEC_INCLUDE_DIR}
)

target_link_libraries(bkk-key-client PRIVATE ${TEEC_LIBRARY})

install(TARGETS bkk-key-client
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES bkk_key_client.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bkk-key-client
)
