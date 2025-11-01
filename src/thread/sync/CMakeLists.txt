add_library(sync INTERFACE)
target_include_directories(sync INTERFACE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(sync INTERFACE Threads::Threads os)

# Alias for convenience
add_library(thread::sync ALIAS sync)
