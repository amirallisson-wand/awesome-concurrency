cmake_minimum_required(VERSION 3.14)
project(awesome_concurrency VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Sanitizer options
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# Check for conflicting sanitizer options
if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "Cannot enable both ASAN and TSAN simultaneously")
endif()

# Apply sanitizer flags
if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_TSAN)
    message(STATUS "ThreadSanitizer enabled")
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=thread)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_subdirectory(src)

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    add_subdirectory(tests)
endif()

option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

add_custom_target(run-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
    DEPENDS mcs_test ticket_lock_test ttas_spinlock_test mutex_test ring_buffer_test fast_ring_buffer_test
)

# Convenience target for running tests with AddressSanitizer
add_custom_target(run-tests-asan
    COMMAND ${CMAKE_COMMAND} -E echo "Building and running tests with AddressSanitizer..."
    COMMAND ${CMAKE_COMMAND} -B ${CMAKE_SOURCE_DIR}/build-asan -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_SOURCE_DIR}/build-asan --parallel
    COMMAND ${CMAKE_COMMAND} -E env ASAN_OPTIONS=detect_leaks=1:symbolize=1:halt_on_error=1 
            ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_SOURCE_DIR}/build-asan --output-on-failure
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running tests with AddressSanitizer..."
)

# Convenience target for running tests with ThreadSanitizer
add_custom_target(run-tests-tsan
    COMMAND ${CMAKE_COMMAND} -E echo "Building and running tests with ThreadSanitizer..."
    COMMAND ${CMAKE_COMMAND} -B ${CMAKE_SOURCE_DIR}/build-tsan -DENABLE_TSAN=ON -DCMAKE_BUILD_TYPE=Debug
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_SOURCE_DIR}/build-tsan --parallel
    COMMAND ${CMAKE_COMMAND} -E env TSAN_OPTIONS=second_deadlock_stack=1:halt_on_error=1
            ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_SOURCE_DIR}/build-tsan --output-on-failure
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running tests with ThreadSanitizer..."
)

add_custom_target(format
    COMMAND find . \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cc" -o -name "*.hh" -o -name "*.h" -o -name "*.cxx" -o -name "*.hxx" \) -exec clang-format -i {} +
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting all C++ source files..."
)
