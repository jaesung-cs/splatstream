set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED COMPONENTS volk)

include(FetchContent)
FetchContent_Declare(
  vk_radix_sort
  GIT_REPOSITORY https://github.com/jaesung-cs/vulkan_radix_sort.git
  GIT_TAG        c4ce1c3688bad92d23c15afd729842054af19ad0  # v0.1.2
)

FetchContent_MakeAvailable(vk_radix_sort)

target_compile_definitions(vk_radix_sort PUBLIC VRDX_USE_VOLK)
target_include_directories(vk_radix_sort PUBLIC ${Vulkan_INCLUDE_DIRS}/Volk)

add_library(vkgs_core SHARED
  src/gaussian_splats.cc
  src/module.cc
  src/rendered_image.cc
  src/sorter.cc
)
add_library(vkgs::core ALIAS vkgs_core)

target_compile_definitions(vkgs_core
  PRIVATE
    VK_NO_PROTOTYPES
)

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
