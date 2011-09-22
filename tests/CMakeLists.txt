add_executable(asebatest
	asebatest.cpp
)

target_link_libraries(asebatest asebacompiler asebavm asebacore ${EXTRA_LIBS})

install(TARGETS asebatest RUNTIME
	DESTINATION bin
	LIBRARY DESTINATION lib
)

# the following tests should succeed
add_test(basic-arithmetic ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/basic-arithmetic.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/basic-arithmetic.txt)
add_test(compound-assignment ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/compound-assignments.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/compound-assignments.txt)
add_test(binary-assignment ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/binary-assignments.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/binary-assignments.txt)
add_test(shift-assignment ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-assignments.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-assignments.txt)
add_test(multiple-logic-op ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/multiple-logic-op.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/multiple-logic-op.txt)
add_test(unicode ${EXECUTABLE_OUTPUT_PATH}/asebatest -d -s ${CMAKE_CURRENT_SOURCE_DIR}/data/unicode.txt)
add_test(optimisation-binary-not ${EXECUTABLE_OUTPUT_PATH}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/optimisation-binary-not.txt)
add_test(for-loop ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/for-loop.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/for-loop.txt)
add_test(while-loop ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/while-loop.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/while-loop.txt)

# the following tests should fail
add_test(division-by-zero-dyn ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/division-by-zero-dyn.txt)
add_test(division-by-zero-static ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/division-by-zero-static.txt)
add_test(chained-conditional ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/chained-conditional.txt)
add_test(implicit-conditional ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/implicit-conditional.txt)
add_test(array-access-out-of-bounds-dyn-over ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-dyn-over.txt)
add_test(array-access-out-of-bounds-dyn-under ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-dyn-under.txt)
add_test(array-access-out-of-bounds-static-over ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-static-over.txt)
add_test(array-access-out-of-bounds-static-under ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-static-under.txt)
add_test(assigning-bool ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/assigning-bool.txt)
