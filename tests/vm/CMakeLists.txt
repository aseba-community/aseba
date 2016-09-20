# test the number of native functions in the vm
add_executable(aseba-test-natives-count
	aseba-test-natives-count.cpp
)
target_link_libraries(aseba-test-natives-count asebacompiler asebavm asebavmdummycallbacks ${ASEBA_CORE_LIBRARIES})
add_test(natives-count ${EXECUTABLE_OUTPUT_PATH}/aseba-test-natives-count)
