cmake_minimum_required(VERSION 3.15)

project(vkgs_example LANGUAGES CXX)

add_subdirectory(.. ./vkgs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# vkgs_exe
add_executable(vkgs_exe
  vkgs_exe.cc
)

target_link_libraries(vkgs_exe
  PRIVATE
    vkgs::api
)

set_target_properties(vkgs_exe PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)

# viewer
add_executable(viewer
  viewer.cc
)

target_link_libraries(viewer
  PRIVATE
    vkgs::viewer
)

set_target_properties(viewer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)
