add_library(parser)
target_sources(parser
	PUBLIC
		FILE_SET cxx_modules
		TYPE CXX_MODULES
		FILES
			parser.cppm
)