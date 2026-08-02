add_library(bkk-screen-client SHARED bkk_screen_client.cpp)

set_target_properties(bkk-screen-client PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

target_include_directories(bkk-screen-client 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/
)

target_link_libraries(bkk-screen-client PRIVATE
	bkk_utils_ipc_uds
)


install(TARGETS bkk-screen-client
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY 
    ../include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/
)
