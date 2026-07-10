add_library(connection)
target_sources(connection
	PUBLIC
		FILE_SET cxx_modules
		TYPE CXX_MODULES
		FILES
			connection.cppm
)
target_link_libraries(connection PRIVATE auth parser storage)