add_subdirectory(connection)
add_subdirectory(msg_dispatcher)
add_subdirectory(parser)
add_subdirectory(storage)
add_subdirectory(analyzer)
add_subdirectory(replay)
add_subdirectory(exporter)

add_library(core INTERFACE)

target_link_libraries(core INTERFACE
	connection
	msg_dispatcher
	parser
	storage
	analyzer
	replay
	exporter
)