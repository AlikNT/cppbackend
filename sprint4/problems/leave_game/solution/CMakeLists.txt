cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(game_model STATIC
	src/geom.h
	src/model_serialization.cpp
	src/model.cpp
	src/app.cpp
	src/json_loader.cpp
	src/extra_data.cpp
	src/logger.cpp
	src/boost_json.cpp
	src/request_handler.cpp
	src/collision_detector.cpp
	src/loot_generator.cpp
	src/tagged.h
	src/infrastructure.cpp
	src/postgres.h
	src/postgres.cpp
	src/tagged_uuid.cpp
	src/tagged_uuid.h
)

target_include_directories(game_model PUBLIC CONAN_PKG::boost)

target_link_libraries(game_model PUBLIC CONAN_PKG::boost Threads::Threads CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
)

add_executable(game_server_tests
	tests/state-serialization-tests.cpp
	tests/model-tests.cpp
	tests/loot_generator_tests.cpp
	tests/collision-detector-tests.cpp
)

target_link_libraries(game_server game_model)
target_link_libraries(game_server_tests CONAN_PKG::catch2 game_model)
