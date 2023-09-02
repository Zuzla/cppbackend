cmake_minimum_required(VERSION 3.11)

project(samples CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(merge src/merge.cpp)

target_include_directories(merge PRIVATE CONAN_PKG::boost)
target_link_libraries(merge PRIVATE CONAN_PKG::boost Threads::Threads)