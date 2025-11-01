add_library(ring_buffer INTERFACE)
target_include_directories(ring_buffer INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(ring_buffer INTERFACE os)
