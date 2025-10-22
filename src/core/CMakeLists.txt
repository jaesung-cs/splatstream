set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)
FetchContent_Declare(
  vk_radix_sort
  GIT_REPOSITORY https://github.com/jaesung-cs/vulkan_radix_sort.git
  GIT_TAG        8ebe9c0b22dc3af35b57cee418971cecf574acfc  # v0.2.0-rc1
)

FetchContent_MakeAvailable(vk_radix_sort)

add_library(vkgs_core SHARED
  src/compute_storage.cc
  src/gaussian_splats.cc
  src/graphics_storage.cc
  src/rendered_image.cc
  src/renderer.cc
  src/sorter.cc
  src/transfer_storage.cc
)
add_library(vkgs::core ALIAS vkgs_core)

target_include_directories(vkgs_core
  PUBLIC
    include/
  PRIVATE
    src/
)

target_link_libraries(vkgs_core
  PUBLIC
    vkgs::gpu
  PRIVATE
    vk_radix_sort
)

target_compile_definitions(vkgs_core PRIVATE VKGS_CORE_EXPORTS)

add_shader(vkgs_core shader/inverse_index.comp inverse_index)
add_shader(vkgs_core shader/parse_ply.comp parse_ply)
add_shader(vkgs_core shader/projection.comp projection)
add_shader(vkgs_core shader/rank.comp rank)
add_shader(vkgs_core shader/splat.frag splat_frag)
add_shader(vkgs_core shader/splat.vert splat_vert)

set_target_properties(vkgs_core PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)
