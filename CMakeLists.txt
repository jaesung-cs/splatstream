cmake_minimum_required(VERSION 3.15)

project(pygs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)
find_package(pybind11 REQUIRED)

# vkgs
add_library(vkgs SHARED
  src/vkgs/vkgs.cc
)

target_include_directories(vkgs
  PUBLIC
    include/
)

target_link_libraries(vkgs
  PRIVATE
    Vulkan::Vulkan
)

target_compile_definitions(vkgs PRIVATE VKGS_EXPORTS)

# vkgs_exe
add_executable(vkgs_exe
  examples/vkgs_exe.cc
)

# pygs_test
pybind11_add_module(pygs_test
  binding/pygs_test.cc
)

target_link_libraries(pygs_test
  PRIVATE
    vkgs
)

install(
  TARGETS vkgs pygs_test
  LIBRARY DESTINATION .
  RUNTIME DESTINATION .
)
