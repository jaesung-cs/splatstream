find_package(Vulkan REQUIRED)

include(FetchContent)

FetchContent_Declare(
  volk
  GIT_REPOSITORY https://github.com/zeux/volk.git
  GIT_TAG        f30088b3f4160810b53e19258dd2f7395e5f0ba3  # vulkan-sdk-1.4.328.1
)
FetchContent_MakeAvailable(volk)

FetchContent_Declare(
  vma
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        1d8f600fd424278486eade7ed3e877c99f0846b1  # v3.3.0
)
FetchContent_MakeAvailable(vma)

# Static library only to load Vulkan functions statically properly.
add_library(vkgs_gpu STATIC
  src/cmd/barrier.cc
  src/cmd/pipeline.cc
  src/details/command_pool.cc
  src/details/command.cc
  src/details/fence_pool.cc
  src/details/fence.cc
  src/details/graphics_pipeline_pool.cc
  src/details/semaphore_pool.cc
  src/details/task_monitor.cc
  src/buffer.cc
  src/compute_pipeline.cc
  src/device.cc
  src/gpu.cc
  src/graphics_pipeline.cc
  src/image.cc
  src/object.cc
  src/pipeline_layout.cc
  src/queue_task.cc
  src/queue.cc
  src/sampler.cc
  src/semaphore.cc
  src/swapchain.cc
  src/task.cc
  src/timer.cc
  src/vma_impl.cc
  src/volk_impl.cc
)
add_library(vkgs::gpu ALIAS vkgs_gpu)

target_include_directories(vkgs_gpu
  PUBLIC
    include/
  PRIVATE
    src/
)

target_link_libraries(vkgs_gpu
  PUBLIC
    Vulkan::Headers
    volk::volk_headers
    vkgs::common
  PRIVATE
    GPUOpen::VulkanMemoryAllocator
)

target_compile_definitions(vkgs_gpu
  PUBLIC
    VKGS_GPU_STATIC
    VK_NO_PROTOTYPES
)

# TODO: Disables too many warnings from vma (MacOS only)
#target_compile_options(vkgs_gpu PRIVATE -Wno-nullability-completeness)

set_target_properties(vkgs_gpu PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  POSITION_INDEPENDENT_CODE ON
)
