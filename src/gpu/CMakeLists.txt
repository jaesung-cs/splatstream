set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED COMPONENTS volk)

add_library(vkgs_gpu SHARED
  src/buffer.cc
  src/command_pool.cc
  src/command.cc
  src/compute_pipeline.cc
  src/device.cc
  src/fence_pool.cc
  src/fence.cc
  src/graphics_pipeline.cc
  src/image.cc
  src/pipeline_layout.cc
  src/queue.cc
  src/semaphore_pool.cc
  src/semaphore.cc
  src/task_monitor.cc
  src/task.cc
  src/volk_impl.cc
  src/vma_impl.cc
)
add_library(vkgs::gpu ALIAS vkgs_gpu)

target_compile_definitions(vkgs_gpu
  PRIVATE
    VK_NO_PROTOTYPES
)

target_include_directories(vkgs_gpu
  PUBLIC
    include/
    ${Vulkan_INCLUDE_DIRS}/Volk
    ${Vulkan_INCLUDE_DIRS}/vma
  PRIVATE
    src/
)

target_link_libraries(vkgs_gpu
  PUBLIC
    Vulkan::Vulkan
)

target_compile_definitions(vkgs_gpu PRIVATE VKGS_GPU_EXPORTS)

set_target_properties(vkgs_gpu PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)
