add_library(frame-stream
  frame_stream.cpp
  frame_stream.hpp
)

target_link_libraries(frame-stream PUBLIC
  project_warnings
  project_options
  GL
  freenect2
)

add_executable(frame-stream-cli
  main.cpp
)
set_target_properties(frame-stream-cli PROPERTIES OUTPUT_NAME "frame-stream")

target_link_libraries(frame-stream-cli PRIVATE
  project_warnings
  project_options
  frame-stream
)
