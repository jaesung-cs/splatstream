cmake_minimum_required(VERSION 3.15)

project(vkgs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(cmake/add_shader.cmake)

add_subdirectory(src/common)
add_subdirectory(src/gpu)
add_subdirectory(src/core)
add_subdirectory(src/viewer)
add_subdirectory(src/vkgs)
