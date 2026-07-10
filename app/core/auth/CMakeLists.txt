add_library(auth)
target_sources(auth
	PUBLIC
		FILE_SET cxx_modules
		TYPE CXX_MODULES
		FILES
			auth.cppm
)
target_link_libraries(auth PRIVATE storage)