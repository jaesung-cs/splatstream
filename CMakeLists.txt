cmake_minimum_required(VERSION 3.15)

project(pygs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(src/core)
add_subdirectory(src/api)

add_subdirectory(examples)
