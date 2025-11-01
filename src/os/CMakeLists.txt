add_library(os INTERFACE)
target_include_directories(os INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_sources(os INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/constants.hpp
)

add_subdirectory(futex)
