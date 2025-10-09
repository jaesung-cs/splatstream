set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED COMPONENTS volk)

include(FetchContent)
FetchContent_Declare(
  vk_radix_sort
  GIT_REPOSITORY https://github.com/jaesung-cs/vulkan_radix_sort.git
  GIT_TAG        231e927f7f360c35c44933dae69c559e6126ccd3  # v0.1.1
)

FetchContent_MakeAvailable(vk_radix_sort)

target_compile_definitions(vk_radix_sort PUBLIC VRDX_USE_VOLK)
target_include_directories(vk_radix_sort PUBLIC ${Vulkan_INCLUDE_DIRS}/Volk)

add_library(vkgs_core SHARED
  src/buffer.cc
  src/command_pool.cc
  src/command.cc
  src/compute_pipeline.cc
  src/descriptor_set_layout.cc
  src/device.cc
  src/fence_pool.cc
  src/fence.cc
  src/gaussian_splats.cc
  src/module.cc
  src/pipeline_layout.cc
  src/queue.cc
  src/semaphore_pool.cc
  src/semaphore.cc
  src/sorter.cc
  src/task_monitor.cc
  src/task.cc
  src/volk_impl.cc
  src/vma_impl.cc
)
add_library(vkgs::core ALIAS vkgs_core)

target_compile_definitions(vkgs_core
  PRIVATE
    VK_NO_PROTOTYPES
)

target_include_directories(vkgs_core
  PUBLIC
    include/
    ${Vulkan_INCLUDE_DIRS}/Volk
    ${Vulkan_INCLUDE_DIRS}/vma
  PRIVATE
    src/
)

target_link_libraries(vkgs_core
  PUBLIC
    Vulkan::Vulkan
  PRIVATE
    vk_radix_sort
)

target_compile_definitions(vkgs_core PRIVATE VKGS_CORE_EXPORTS)

add_shader(vkgs_core shader/parse_ply.comp parse_ply)

set_target_properties(vkgs_core PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)
