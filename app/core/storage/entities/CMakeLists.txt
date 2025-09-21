add_library(entities)
target_sources(entities
	PUBLIC
		FILE_SET cxx_modules
		TYPE CXX_MODULES
		FILES
			entities.cppm
)