add_executable(test_dbus test_main.c)

target_link_libraries(test_dbus
	PRIVATE
		bkk-utils-dbus
)


add_executable(test_broadcast_server test_broadcast_server.c)

target_link_libraries(test_broadcast_server
	PRIVATE
		bkk-utils-dbus
)


add_executable(test_broadcast_client test_broadcast_client.c)

target_link_libraries(test_broadcast_client
	PRIVATE
		bkk-utils-dbus
)


