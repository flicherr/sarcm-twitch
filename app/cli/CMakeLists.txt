add_library(cli)
target_sources(cli
	PUBLIC
		FILE_SET cxx_modules
		TYPE CXX_MODULES
		FILES
			cli.cppm
)
target_link_libraries(cli PRIVATE connection analysis parser)