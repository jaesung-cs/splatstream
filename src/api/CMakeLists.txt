set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(vkgs_api SHARED
  src/buffer_impl.cc
  src/buffer.cc
  src/module_impl.cc
  src/module.cc
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
