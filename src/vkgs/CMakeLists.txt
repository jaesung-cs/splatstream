set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(vkgs_vkgs SHARED
  src/engine.cc
  src/gaussian_splats.cc
  src/rendering_task.cc
)
add_library(vkgs::vkgs ALIAS vkgs_vkgs)

target_include_directories(vkgs_vkgs
  PUBLIC
    include/
  PRIVATE
    src/
)

target_link_libraries(vkgs_vkgs
  PRIVATE
    vkgs::core
    vkgs::viewer
)

target_compile_definitions(vkgs_vkgs PRIVATE VKGS_EXPORTS)

set_target_properties(vkgs_vkgs PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)
