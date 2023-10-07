cmake_minimum_required(VERSION 3.0.0)
project(v0 CXX)
set(CMAKE_CXX_STANDARD 20)

add_executable(v0
    config.cpp
    config.h
    dotgen.cpp
    dotgen.h
    graph.cpp
    graph.h
    main.cpp
    readfile.cpp
    readfile.h
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

add_custom_command(
        TARGET ${PROJECT_NAME}  POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/inputs
                ${CMAKE_CURRENT_BINARY_DIR}/inputs
)

add_custom_command(
        TARGET ${PROJECT_NAME}  POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/pathalizer.conf
                ${CMAKE_CURRENT_BINARY_DIR}/pathalizer.conf
)