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

# Fix rpath
# See https://scikit-build-core.readthedocs.io/en/latest/guide/dynamic_link.html.
if(APPLE)
  set(origin_token "@loader_path")
else()
  set(origin_token "$ORIGIN")
endif()
set_property(TARGET pygs PROPERTY INSTALL_RPATH
  "${origin_token}/"
)

install(
  TARGETS vkgs_api vkgs_core pygs
  LIBRARY DESTINATION pygs
  RUNTIME DESTINATION pygs
)
