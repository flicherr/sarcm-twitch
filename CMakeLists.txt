cmake_minimum_required(VERSION 4.0)
project(sarcm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(COMMON_FLAGS
	-Wall
	-Wextra
	-Werror
	-pedantic
	-fmodules
)

add_subdirectory(app/core/auth)
add_subdirectory(app/core/parser)
add_subdirectory(app/core/storage/entities)
add_subdirectory(app/core/storage)
add_subdirectory(app/core/connection)
add_executable(${PROJECT_NAME} app/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE
	auth
	storage
	connection
)

target_compile_options(${PROJECT_NAME} PRIVATE ${COMMON_FLAGS})