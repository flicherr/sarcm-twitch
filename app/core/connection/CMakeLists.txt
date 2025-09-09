add_library(connection INTERFACE
#	tcp/TCPConnection.cpp
#	websocket/WSConnection.cpp
)
target_include_directories(connection INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
#	${CMAKE_CURRENT_SOURCE_DIR}/tcp
#	${CMAKE_CURRENT_SOURCE_DIR}/websocket
)