# test that invalid utf8 strings do not generate memory trashes
add_executable(aseba-test-invalid-utf8
	aseba-test-invalid-utf8.cpp
)
target_link_libraries(aseba-test-invalid-utf8 ${ASEBA_CORE_LIBRARIES})
add_test(aseba-test-invalid-utf8 ${valgrind_FOUND} --error-exitcode=1 ${CMAKE_CURRENT_BINARY_DIR}/aseba-test-invalid-utf8)
