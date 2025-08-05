cmake_minimum_required(VERSION 3.15)

project(pygs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)

add_executable(pygs_exe
  examples/pygs_exe.cc
)
