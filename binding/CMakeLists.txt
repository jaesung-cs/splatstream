cmake_minimum_required(VERSION 3.15)

project(splatstream_binding LANGUAGES C CXX)

find_package(pybind11 REQUIRED)

add_subdirectory(.. ./build/vkgs)

# splatstream
pybind11_add_module(splatstream
  binding.cc
)
set_target_properties(splatstream PROPERTIES OUTPUT_NAME _core)

target_link_libraries(splatstream
  PRIVATE
    vkgs::api
)

# Fix rpath for shared libraries
# See https://scikit-build-core.readthedocs.io/en/latest/guide/dynamic_link.html.
if(APPLE)
  set(origin_token "@loader_path")
else()
  set(origin_token "$ORIGIN")
endif()
set_property(TARGET splatstream PROPERTY INSTALL_RPATH
  "${origin_token}/"
)
set_property(TARGET vkgs_api PROPERTY INSTALL_RPATH
  "${origin_token}/"
)

install(TARGETS vkgs_api vkgs_core splatstream DESTINATION splatstream)
