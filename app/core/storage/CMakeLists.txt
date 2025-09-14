add_library(storage)
target_sources(storage
	PUBLIC
		FILE_SET modules_public
		TYPE CXX_MODULES
		FILES
			storage.cppm
)
target_link_libraries(storage PRIVATE entities sqlite3)