set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED COMPONENTS volk)

add_library(vkgs_core SHARED
  src/buffer.cc
  src/command.cc
  src/module.cc
  src/semaphore.cc
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
    Vulkan::volk
)

target_compile_definitions(vkgs_core PRIVATE VKGS_CORE_EXPORTS)
