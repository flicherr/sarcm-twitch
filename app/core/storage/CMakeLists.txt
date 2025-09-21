add_library(storage)
target_sources(storage
	PUBLIC
		FILE_SET cxx_modules
		TYPE CXX_MODULES
		FILES
			storage.cppm
)
target_link_libraries(storage PRIVATE entities sqlite3)