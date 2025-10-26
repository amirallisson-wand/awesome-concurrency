add_library(util INTERFACE)
target_include_directories(util INTERFACE
    ${CMAKE_SOURCE_DIR}/src
)

# Alias for convenience
add_library(thread::util ALIAS util)
