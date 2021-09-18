cmake_minimum_required(VERSION 3.15)

# Based off of https://github.com/lefticus/cpp_starter_project
project(kinect-time CXX)
include(cmake/StandardProjectSettings.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)

add_library(project_warnings INTERFACE)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

# Disable for now, try as third_party
option(KINECT_TRITON_INFERENCE_SERVER "Use nvidia's triton inference client" OFF)

include(cmake/StaticAnalyzers.cmake)

set(CONAN_EXTRA_REQUIRES "")
set(CONAN_EXTRA_OPTIONS "")

add_subdirectory(third_party/libfreenect2)
add_subdirectory(kinect)

if(ENABLE_TESTING)
  find_package(GTest REQUIRED)
  enable_testing()
  message("Configuring tests...")
  add_subdirectory(test)
endif()