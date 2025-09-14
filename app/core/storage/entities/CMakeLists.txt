add_library(entities)
target_sources(entities
	PUBLIC
		FILE_SET modules_public
		TYPE CXX_MODULES
		FILES
			entities.cppm
)