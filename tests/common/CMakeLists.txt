# test that invalid utf8 strings do not generate memory trashes
add_executable(aseba-test-invalid-utf8
	aseba-test-invalid-utf8.cpp
)
target_link_libraries(aseba-test-invalid-utf8 asebacommon)
add_test(NAME aseba-test-invalid-utf8 COMMAND ${valgrind_FOUND} --error-exitcode=1 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/aseba-test-invalid-utf8)
