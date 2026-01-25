include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        3912b3d9a9c1b3f17431aebafd86d2f40ee6e59c  # v1.92.5-docking
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
  imguizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
  GIT_TAG        71f14292205c3317122b39627ed98efce137086a  # 20251105, v1.92.5 WIP
)
FetchContent_MakeAvailable(imguizmo)

FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG        4707b245fbcd69075b1a8a74fa8d2435561b3134  # v0.17
)
FetchContent_MakeAvailable(implot)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        7b6aead9fb88b3623e3b3725ebb42670cbe4c579  # 3.4
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

add_library(vkgs_viewer SHARED
  src/camera.cc
  src/context.cc
  src/imgui_texture.cc
  src/pose_spline.cc
  src/storage.cc
  src/util.cc
  src/viewer.cc
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
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
    ${implot_SOURCE_DIR}
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

add_shader(vkgs_viewer shader/screen.vert screen_vert)
add_shader(vkgs_viewer shader/blend_color.frag blend_color_frag)
add_shader(vkgs_viewer shader/blend_depth.frag blend_depth_frag)
add_shader(vkgs_viewer shader/color.vert color_vert)
add_shader(vkgs_viewer shader/color.frag color_frag)
add_shader(vkgs_viewer shader/depth.vert depth_vert)
add_shader(vkgs_viewer shader/depth.frag depth_frag)

set_target_properties(vkgs_viewer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  POSITION_INDEPENDENT_CODE ON
)
