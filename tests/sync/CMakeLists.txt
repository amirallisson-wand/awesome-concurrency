add_executable(mcs_test mcs_test.cpp)
target_link_libraries(mcs_test PRIVATE sync GTest::gtest_main)

add_executable(ticket_lock_test ticket_lock_test.cpp)
target_link_libraries(ticket_lock_test PRIVATE sync GTest::gtest_main)

add_executable(ttas_spinlock_test ttas_spinlock_test.cpp)
target_link_libraries(ttas_spinlock_test PRIVATE sync GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(mcs_test)
gtest_discover_tests(ticket_lock_test)
gtest_discover_tests(ttas_spinlock_test)
