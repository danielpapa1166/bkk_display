cmake_minimum_required(VERSION 3.16)

add_executable(network_manager
	nm_main.c
	wpa_file_handler.c
	networkd_status.c
	supplicant_handler.c
)


target_link_libraries(network_manager PRIVATE
	rbuflogd_producer
	systemd
	cjson
)

target_compile_options(network_manager
	PRIVATE -Wall -Wextra -pedantic -g -O0)



install(TARGETS network_manager RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
