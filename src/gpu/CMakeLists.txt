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
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        a532f5b1cf27d6a3c099437e6959cf7e398a0a67  # 1.0.2
)
FetchContent_MakeAvailable(glm)

# Static library only to load Vulkan functions statically properly.
add_library(vkgs_gpu STATIC
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
    Vulkan::Vulkan
    volk::volk_headers
    GPUOpen::VulkanMemoryAllocator
    glm::glm
)

target_compile_definitions(vkgs_gpu PUBLIC VKGS_GPU_STATIC)

# TODO: Disables too many warnings from vma (MacOS only)
#target_compile_options(vkgs_gpu PRIVATE -Wno-nullability-completeness)

set_target_properties(vkgs_gpu PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)
