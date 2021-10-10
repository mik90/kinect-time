# Kinect wrapper lib

add_library(kinect
  kinect.cpp
)

target_include_directories(kinect PUBLIC
 ${CMAKE_CURRENT_LIST_DIR}

 # Should probably do this the right way through cmake targets. There is a better way, right?
 ${libfreenect2_SOURCE_DIR}/include
 ${libfreenect2_BINARY_DIR}
)

set_target_properties(kinect
  PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

target_link_libraries(kinect PUBLIC
  project_warnings
  project_options
  GL
  freenect2
)

# Test drivers

add_executable(kinect-test-driver
  test-driver.cpp
)

set_target_properties(kinect-test-driver 
  PROPERTIES
  OUTPUT_NAME "test-driver"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_link_libraries(kinect-test-driver PRIVATE
  project_warnings
  project_options
  kinect 
)
