cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Adding the static library
add_library(game_server_lib STATIC
	src/model.cpp
	src/app.cpp
	src/json_loader.cpp
	src/extra_data.cpp
	src/loot_generator.cpp
	src/logger.cpp
	src/boost_json.cpp
	src/request_handler.cpp
)

# Adding external libraries visibility
target_include_directories(game_server_lib PUBLIC CONAN_PKG::boost)
target_link_libraries(game_server_lib PUBLIC CONAN_PKG::boost Threads::Threads)

# Adding targets, specifying only their own files
add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/tagged.h
)

add_executable(game_server_tests
	tests/model-tests.cpp
	tests/loot_generator_tests.cpp
)

# Adding dependencies of targets on the static library
target_link_libraries(game_server game_server_lib)
target_link_libraries(game_server_tests CONAN_PKG::catch2 game_server_lib)
