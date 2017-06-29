add_executable(aseba-test-msg aseba-test-msg.cpp)
target_link_libraries(aseba-test-msg ${ASEBA_CORE_LIBRARIES})

# the following tests should succeed
add_test(msg ${EXECUTABLE_OUTPUT_PATH}/aseba-test-msg)
