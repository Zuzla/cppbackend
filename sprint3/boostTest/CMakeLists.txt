enable_testing()

cmake_minimum_required(VERSION 3.11)

project(samples CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

add_executable(boost_test_example
    tests/boost_test_main.cpp
    src/leap_year.h
    src/leap_year.cpp
) 

# Линкуем тестирующее приложение с Boost, так как Boost.Test требует компоновки
target_link_libraries(boost_test_example PRIVATE CONAN_PKG::boost) 