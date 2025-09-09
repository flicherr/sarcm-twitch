cmake_minimum_required(VERSION 4.0)
project(twitch-sarc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

set(COMMON_FLAGS
	-Wall
	-Wextra
	-Werror
	-pedantic
)

add_executable(${PROJECT_NAME} app/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE
	core
	cli
)

target_compile_options(${PROJECT_NAME} PRIVATE ${COMMON_FLAGS})