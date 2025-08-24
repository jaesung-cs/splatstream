cmake_minimum_required(VERSION 3.15)

project(pygs_binding LANGUAGES C CXX)

find_package(pybind11 REQUIRED)

add_subdirectory(.. ./build/vkgs)

# pygs
pybind11_add_module(pygs
  pygs.cc
)
set_target_properties(pygs PROPERTIES OUTPUT_NAME _core)

target_link_libraries(pygs
  PRIVATE
    vkgs::api
)

install(
  TARGETS vkgs_api pygs
  LIBRARY DESTINATION pygs
  RUNTIME DESTINATION pygs
)
