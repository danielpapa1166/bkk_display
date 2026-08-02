add_library(bkk-utils-dbus 
    SHARED 
        bkk_dbus.c
        bkk_dbus_broadcast_client.c
        bkk_dbus_broadcast_server.c
)

set_target_properties(bkk-utils-dbus PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)


find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)


target_include_directories(bkk-utils-dbus
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/
)

target_link_libraries(bkk-utils-dbus
    PUBLIC
        PkgConfig::DBUS
)

install(TARGETS bkk-utils-dbus
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY 
    ../include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bkk_utils/ 
)

install(DIRECTORY 
    ../common/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bkk_utils/
)
