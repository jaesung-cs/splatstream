set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        9a5d5c45f54b1301ea471622eddede70384243af  # v1.92.4
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
  imguizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
  GIT_TAG        71f14292205c3317122b39627ed98efce137086a  # 20251105, v1.92.5 WIP
)
FetchContent_MakeAvailable(imguizmo)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        7b6aead9fb88b3623e3b3725ebb42670cbe4c579  # 3.4
)
FetchContent_MakeAvailable(glfw)

add_library(vkgs_viewer SHARED
  src/camera.cc
  src/context.cc
  src/storage.cc
  src/viewer.cc
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)
add_library(vkgs::viewer ALIAS vkgs_viewer)

target_include_directories(vkgs_viewer
  PUBLIC
    include/
  PRIVATE
    src/
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imguizmo_SOURCE_DIR}
)

target_link_libraries(vkgs_viewer
  PRIVATE
    vkgs::core
    vkgs::gpu
    glfw
)

target_compile_definitions(vkgs_viewer
  PRIVATE
    VKGS_VIEWER_EXPORTS
)

add_shader(vkgs_viewer shader/color.frag color_frag)
add_shader(vkgs_viewer shader/color.vert color_vert)

set_target_properties(vkgs_viewer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  POSITION_INDEPENDENT_CODE ON
)
