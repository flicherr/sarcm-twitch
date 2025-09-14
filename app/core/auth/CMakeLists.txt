add_library(auth)
target_sources(auth
	PUBLIC
		FILE_SET modules_public
		TYPE CXX_MODULES
		FILES
			auth.cppm
)