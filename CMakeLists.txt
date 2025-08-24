cmake_minimum_required(VERSION 3.15)

project(pygs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(src/core)
add_subdirectory(src/api)

# vkgs_exe
add_executable(vkgs_exe
  examples/vkgs_exe.cc
)

target_link_libraries(vkgs_exe
  PRIVATE
    vkgs::api
)
