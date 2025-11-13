set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        7b6aead9fb88b3623e3b3725ebb42670cbe4c579  # 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  vk_radix_sort
  GIT_REPOSITORY https://github.com/jaesung-cs/vulkan_radix_sort.git
  GIT_TAG        cde8ba29b9c97e99e81bfd66c1022ce126660c25  # v0.2.0
)
FetchContent_MakeAvailable(vk_radix_sort)

# Static library only to load Vulkan functions statically properly.
add_library(vkgs_gpu STATIC
  src/cmd/barrier.cc
  src/cmd/pipeline.cc
  src/cmd/queue_submission.cc
  src/buffer.cc
  src/command_pool.cc
  src/command.cc
  src/compute_pipeline.cc
  src/device.cc
  src/fence_pool.cc
  src/fence.cc
  src/gpu.cc
  src/graphics_pipeline.cc
  src/image.cc
  src/pipeline_layout.cc
  src/queue.cc
  src/semaphore_pool.cc
  src/semaphore.cc
  src/task_monitor.cc
  src/task.cc
  src/timer.cc
  src/vma_impl.cc
  src/volk_impl.cc
  src/vrdx_impl.cc
)
add_library(vkgs::gpu ALIAS vkgs_gpu)

target_compile_definitions(vkgs_gpu
  PUBLIC
    VK_NO_PROTOTYPES
)

target_include_directories(vkgs_gpu
  PUBLIC
    include/
  PRIVATE
    src/
)

target_link_libraries(vkgs_gpu
  PUBLIC
    volk::volk_headers
    GPUOpen::VulkanMemoryAllocator
    glfw
    vk_radix_sort
)

target_compile_definitions(vkgs_gpu PUBLIC VKGS_GPU_STATIC)

# TODO: Disables too many warnings from vma (MacOS only)
#target_compile_options(vkgs_gpu PRIVATE -Wno-nullability-completeness)

set_target_properties(vkgs_gpu PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  POSITION_INDEPENDENT_CODE ON
)
