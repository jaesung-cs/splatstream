set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        a532f5b1cf27d6a3c099437e6959cf7e398a0a67  # 1.0.2
)
FetchContent_MakeAvailable(glm)

add_library(vkgs_core SHARED
  src/compute_storage.cc
  src/core.cc
  src/gaussian_splats.cc
  src/graphics_storage.cc
  src/rendering_task.cc
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
    glm::glm
  PRIVATE
    vkgs::gpu
)

target_compile_definitions(vkgs_core PRIVATE VKGS_CORE_EXPORTS)

add_shader(vkgs_core shader/inverse_index.comp inverse_index)
add_shader(vkgs_core shader/parse_ply.comp parse_ply)
add_shader(vkgs_core shader/parse_data.comp parse_data)
add_shader(vkgs_core shader/projection.comp projection)
add_shader(vkgs_core shader/rank.comp rank)
add_shader(vkgs_core shader/splat_background.frag splat_background_frag)
add_shader(vkgs_core shader/splat_background.vert splat_background_vert)
add_shader(vkgs_core shader/splat.frag splat_frag)
add_shader(vkgs_core shader/splat.vert splat_vert)

set_target_properties(vkgs_core PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  POSITION_INDEPENDENT_CODE ON
)
