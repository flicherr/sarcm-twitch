add_library(connection)
target_sources(connection
	PUBLIC
		FILE_SET modules_public
		TYPE CXX_MODULES
		FILES
			connection.cppm
)
target_link_libraries(connection PRIVATE auth parser)