add_executable(network-manager-dbus-test
	network_manager_dbus_test.c
)

target_include_directories(network-manager-dbus-test PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(network-manager-dbus-test PRIVATE
	bkk-utils-dbus
)

target_compile_options(network-manager-dbus-test
	PRIVATE -Wall -Wextra -pedantic
)

install(TARGETS network-manager-dbus-test RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})