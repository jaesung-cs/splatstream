cmake_minimum_required(VERSION 3.15)

project(pygs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED COMPONENTS volk)
find_package(pybind11 REQUIRED)

# vkgs
add_library(vkgs SHARED
  src/vkgs/impl/module_impl.cc
  src/vkgs/impl/buffer_impl.cc
  src/vkgs/buffer.cc
  src/vkgs/module.cc
  src/vkgs/volk_impl.cc
  src/vkgs/vma_impl.cc
)

target_compile_definitions(vkgs
  PRIVATE
    VK_NO_PROTOTYPES
)

target_include_directories(vkgs
  PUBLIC
    include/
  PRIVATE
    ${Vulkan_INCLUDE_DIRS}/Volk
    ${Vulkan_INCLUDE_DIRS}/vma
    src/
)

target_link_libraries(vkgs
  PRIVATE
    Vulkan::Vulkan
    Vulkan::volk
)

target_compile_definitions(vkgs PRIVATE VKGS_EXPORTS)

# vkgs_exe
add_executable(vkgs_exe
  examples/vkgs_exe.cc
)

target_link_libraries(vkgs_exe
  PRIVATE
    vkgs
)

# pygs
pybind11_add_module(pygs
  binding/pygs.cc
)
set_target_properties(pygs PROPERTIES OUTPUT_NAME _core)

target_link_libraries(pygs
  PRIVATE
    vkgs
)

install(
  TARGETS vkgs pygs
  LIBRARY DESTINATION pygs
  RUNTIME DESTINATION pygs
)
