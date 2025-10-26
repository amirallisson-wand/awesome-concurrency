add_executable(mcs_test mcs_test.cpp)
target_link_libraries(mcs_test PRIVATE sync GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(mcs_test)
