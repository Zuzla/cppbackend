cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
project(game_server_tests CXX)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.81.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(MyLib STATIC 
	src/constants.h
	src/model.h
	src/model.cpp
	src/database.h
	src/database.cpp
	src/application.h
	src/application.cpp
	src/serialization.h
	src/serialization.cpp
	src/loot_generator.h
	src/loot_generator.cpp
	src/collision_detector.h
	src/collision_detector.cpp
	src/responce_api.h
	src/responce_api.cpp
	src/loots.h
	src/loots.cpp
	src/player.cpp
	src/player.h
	src/tagged.h
	src/geom.h
)

target_include_directories(MyLib PUBLIC ${ZLIB_INCLUDES} CONAN_PKG::libpq CONAN_PKG::libpqxx)
target_link_libraries(MyLib PUBLIC ${BOOST_LIB} ${ZLIB_LIB} CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h	
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/logging_request_handler.h	
	src/ticker.cpp
	src/ticker.h
)
target_include_directories(game_server PUBLIC CONAN_PKG::boost )
target_link_libraries(game_server CONAN_PKG::boost) 

add_executable(game_server_tests
    tests/loot_generator_tests.cpp
    tests/collision_detector_tests.cpp
)
target_include_directories(game_server_tests PUBLIC  CONAN_PKG::boost)
target_link_libraries(game_server_tests CONAN_PKG::catch2 CONAN_PKG::boost Threads::Threads ) 

catch_discover_tests(game_server_tests)

target_link_libraries(game_server MyLib)
target_link_libraries(game_server_tests ${CATCH2_LIB} MyLib) 