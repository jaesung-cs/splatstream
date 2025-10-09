set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(vkgs_api SHARED
  src/gaussian_splats_impl.cc
  src/gaussian_splats.cc
  src/module_impl.cc
  src/module.cc
  src/rendered_image_impl.cc
  src/rendered_image.cc
)
add_library(vkgs::api ALIAS vkgs_api)

target_include_directories(vkgs_api
  PUBLIC
    include/
  PRIVATE
    src/
)

target_link_libraries(vkgs_api
  PRIVATE
    vkgs::core
)

target_compile_definitions(vkgs_api PRIVATE VKGS_EXPORTS)

set_target_properties(vkgs_api PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)
