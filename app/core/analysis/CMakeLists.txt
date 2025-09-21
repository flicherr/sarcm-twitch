add_library(analysis)
target_sources(analysis
	PUBLIC
		FILE_SET cxx_modules
		TYPE CXX_MODULES
		FILES
			analysis.cppm
)
target_link_libraries(analysis PRIVATE storage parser)