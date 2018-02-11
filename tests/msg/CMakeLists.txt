add_executable(aseba-test-msg aseba-test-msg.cpp)
target_link_libraries(aseba-test-msg asebacommon)

# the following tests should succeed
add_test(msg aseba-test-msg)
