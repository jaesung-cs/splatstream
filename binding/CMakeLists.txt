cmake_minimum_required(VERSION 3.15)

project(splatstream_binding LANGUAGES C CXX)

add_subdirectory(.. ./build/vkgs)

include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        f5fbe867d2d26e4a0a9177a51f6e568868ad3dc8  # v3.0.1
)
FetchContent_MakeAvailable(pybind11)

# splatstream
pybind11_add_module(splatstream
  binding.cc
)
set_target_properties(splatstream PROPERTIES OUTPUT_NAME _core)

target_link_libraries(splatstream
  PRIVATE
    vkgs::vkgs
)

# Fix rpath for shared libraries
# See https://scikit-build-core.readthedocs.io/en/latest/guide/dynamic_link.html.
if(APPLE)
  set(origin_token "@loader_path")
else()
  set(origin_token "$ORIGIN")
endif()
set_property(
  TARGET splatstream vkgs_vkgs vkgs_core vkgs_gpu
  PROPERTY INSTALL_RPATH "${origin_token}/"
)

install(TARGETS splatstream vkgs_vkgs vkgs_core vkgs_gpu DESTINATION splatstream)
