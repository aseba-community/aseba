# test that invalid utf8 strings do not generate memory trashes
add_executable(aseba-test-invalid-utf8
	aseba-test-invalid-utf8.cpp
)
target_link_libraries(aseba-test-invalid-utf8 asebacommon)
add_test(NAME aseba-test-invalid-utf8 COMMAND aseba-test-invalid-utf8)

add_executable(tst_compiler_utf8 utf8.cpp)
add_test(NAME tst_compiler_utf8 COMMAND tst_compiler_utf8)
target_link_libraries(tst_compiler_utf8 asebacommon catch2)
