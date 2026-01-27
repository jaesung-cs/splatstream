find_package(Vulkan REQUIRED)

include(FetchContent)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        a532f5b1cf27d6a3c099437e6959cf7e398a0a67  # 1.0.2
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
  vk_radix_sort
  GIT_REPOSITORY https://github.com/jaesung-cs/vulkan_radix_sort.git
  GIT_TAG        dd6e1f3930b1ed633f327c0736d44cd562588493  # v0.2.1
)
FetchContent_MakeAvailable(vk_radix_sort)

add_library(vkgs_core STATIC
  src/details/compute_storage.cc
  src/details/graphics_storage.cc
  src/details/sorter.cc
  src/gaussian_splats.cc
  src/parser.cc
  src/renderer.cc
  src/rendering_task.cc
  src/screen_splats.cc
  src/vrdx_impl.cc
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
    glm::glm
    vkgs::gpu
  PRIVATE
    vk_radix_sort
)

target_compile_definitions(vkgs_core
  PUBLIC
    VKGS_CORE_STATIC
  PRIVATE
    VRDX_USE_VOLK
)

add_shader(vkgs_core shader/inverse_index.comp inverse_index)
add_shader(vkgs_core shader/parse_ply.comp parse_ply)
add_shader(vkgs_core shader/parse_data.comp parse_data)
add_shader(vkgs_core shader/projection.comp projection)
add_shader(vkgs_core shader/rank.comp rank)
add_shader(vkgs_core shader/splat_color.frag splat_color_frag)
add_shader(vkgs_core shader/splat_color.vert splat_color_vert)
add_shader(vkgs_core shader/splat_depth.frag splat_depth_frag)
add_shader(vkgs_core shader/splat_depth.vert splat_depth_vert)

set_target_properties(vkgs_core PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  POSITION_INDEPENDENT_CODE ON
)
