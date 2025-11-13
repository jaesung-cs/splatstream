set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        9a5d5c45f54b1301ea471622eddede70384243af  # v1.92.4
)
FetchContent_MakeAvailable(imgui)

add_library(vkgs_viewer SHARED
  src/viewer.cc
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
add_library(vkgs::viewer ALIAS vkgs_viewer)

target_include_directories(vkgs_viewer
  PUBLIC
    include/
  PRIVATE
    src/
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(vkgs_viewer
  PRIVATE
    vkgs::core
    vkgs::gpu
)

target_compile_definitions(vkgs_viewer
  PRIVATE
    VKGS_VIEWER_EXPORTS
    IMGUI_IMPL_VULKAN_USE_VOLK
)

set_target_properties(vkgs_viewer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  POSITION_INDEPENDENT_CODE ON
)
