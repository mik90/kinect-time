add_library(kinect
  kinect.cpp
  kinect.hpp
)

target_include_directories(kinect PUBLIC
 ${CMAKE_CURRENT_LIST_DIR}

 # Should probably do this the right way through cmake targets. There is a better way, right?
 ${libfreenect2_SOURCE_DIR}/include
 ${libfreenect2_BINARY_DIR}
)

target_link_libraries(kinect PUBLIC
  project_warnings
  project_options
  GL
  freenect2
)
