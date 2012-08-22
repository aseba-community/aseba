add_executable(asebatest
	asebatest.cpp
)

target_link_libraries(asebatest asebacompiler asebavm asebacore ${EXTRA_LIBS})

install(TARGETS asebatest RUNTIME
	DESTINATION bin
	LIBRARY DESTINATION lib
)

add_executable(aseba-test-natives-count
	aseba-test-natives-count.cpp
)

target_link_libraries(aseba-test-natives-count asebavm)

install(TARGETS aseba-test-natives-count RUNTIME
	DESTINATION bin
	LIBRARY DESTINATION lib
)

# set the number of test loops for the fuzzy test
set(fuzzy_loop "500")

# the following tests should succeed
add_test(natives-count ${EXECUTABLE_OUTPUT_PATH}/aseba-test-natives-count)
add_test(basic-arithmetic ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/basic-arithmetic.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/basic-arithmetic.txt)
add_test(basic-arithmetic-vector ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/basic-arithmetic-vector.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/basic-arithmetic-vector.txt)
add_test(advanced-arithmetic ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/advanced-arithmetic.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/advanced-arithmetic.txt)
add_test(advanced-arithmetic-vector ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/advanced-arithmetic-vector.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/advanced-arithmetic-vector.txt)
add_test(binary-op ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/binary-op.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/binary-op.txt)
add_test(shift-op ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-op.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-op.txt)
add_test(compound-assignment ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/compound-assignments.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/compound-assignments.txt)
add_test(compound-assignment-vector ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/compound-assignments-vector.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/compound-assignments-vector.txt)
add_test(binary-assignment ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/binary-assignments.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/binary-assignments.txt)
add_test(shift-assignment ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-assignments.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-assignments.txt)
add_test(shift-assignment-vector ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-assignments-vector.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-assignments-vector.txt)
add_test(multiple-logic-op ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/multiple-logic-op.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/multiple-logic-op.txt)
add_test(unicode ${EXECUTABLE_OUTPUT_PATH}/asebatest -d -s ${CMAKE_CURRENT_SOURCE_DIR}/data/unicode.txt)
add_test(optimisation-binary-not ${EXECUTABLE_OUTPUT_PATH}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/optimisation-binary-not.txt)
add_test(for-loop ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/for-loop.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/for-loop.txt)
add_test(for-loop-vector ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/for-loop-vector.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/for-loop-vector.txt)
add_test(while-loop ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/while-loop.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/while-loop.txt)
add_test(while-loop-vector ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/while-loop-vector.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/while-loop-vector.txt)
add_test(when-conditional ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/when-conditional.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/when-conditional.txt)
add_test(comments ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/comments.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/comments.txt)
add_test(subroutine ${EXECUTABLE_OUTPUT_PATH}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/subroutine.txt)
add_test(array-post-increment ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/array-post-increment.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/array-post-increment.txt)
add_test(array-constant-access ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/array-constant-access.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/array-constant-access.txt)
add_test(vardef ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef.txt)
add_test(vardef-compat ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef-compat.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef-compat.txt)
add_test(vardef-constant-size ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef-constant-size.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef-constant-size.txt)
add_test(general-tuple ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/general-tuple.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/general-tuple.txt)
add_test(assignments ${EXECUTABLE_OUTPUT_PATH}/asebatest --memcmp ${CMAKE_CURRENT_SOURCE_DIR}/data/assignments.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/assignments.txt)
add_test(events ${EXECUTABLE_OUTPUT_PATH}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/events.txt)
add_test(general-tuple-events ${EXECUTABLE_OUTPUT_PATH}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/general-tuple-events.txt)
add_test(native-function ${EXECUTABLE_OUTPUT_PATH}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/native-function.txt)
add_test(general-tuple-native-function ${EXECUTABLE_OUTPUT_PATH}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/general-tuple-native-function.txt)
add_test(var-def-compat-issue135 ${EXECUTABLE_OUTPUT_PATH}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/var-def-compat-issue135.txt)


# the following tests should fail
add_test(division-by-zero-dyn ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/division-by-zero-dyn.txt)
add_test(division-by-zero-static ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/division-by-zero-static.txt)
add_test(chained-conditional ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/chained-conditional.txt)
add_test(implicit-conditional ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/implicit-conditional.txt)
add_test(array-access-out-of-bounds-dyn-over ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-dyn-over.txt)
add_test(array-access-out-of-bounds-dyn-under ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-dyn-under.txt)
add_test(array-access-out-of-bounds-static-over ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-static-over.txt)
add_test(array-access-out-of-bounds-static-under ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-static-under.txt)
add_test(vector-access-out-of-bounds-static-over ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/vector-access-out-of-bounds-static-over.txt)
add_test(vector-access-out-of-bounds-static-under ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/vector-access-out-of-bounds-static-under.txt)
add_test(vector-access-two-expr ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/vector-access-two-expr.txt)
add_test(assigning-bool ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/assigning-bool.txt)
add_test(inconsistent-input1 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/inconsistent-input1.txt)
add_test(inconsistent-input2 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/inconsistent-input2.txt)
add_test(inconsistent-input3 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/inconsistent-input3.txt)
add_test(inconsistent-input4 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/inconsistent-input4.txt)
add_test(assignments-fail1 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/assignments-fail1.txt)
add_test(assignments-fail2 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/assignments-fail2.txt)
add_test(assignments-fail3 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/assignments-fail3.txt)
add_test(assignments-fail4 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/assignments-fail4.txt)
add_test(vardef-fail1 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef-fail1.txt)
add_test(vardef-fail2 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef-fail2.txt)
add_test(vardef-fail3 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef-fail3.txt)
add_test(vardef-compat-fail1 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef-compat-fail1.txt)
add_test(vardef-not-constant-size ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/vardef-not-constant-size.txt)
add_test(out-of-memory1 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/out-of-memory1.txt)
add_test(out-of-memory2 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/out-of-memory2.txt)
add_test(out-of-memory-temp1 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/out-of-memory-temp1.txt)
add_test(out-of-memory-temp2 ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/out-of-memory-temp2.txt)
add_test(if-condition-vector ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/if-condition-vector.txt)
add_test(for-loop-condition-vector ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/for-loop-condition-vector.txt)
add_test(constant-namespace-collision ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/constant-namespace-collision.txt)
add_test(array-constant-access-fail ${EXECUTABLE_OUTPUT_PATH}/asebatest --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/array-constant-access-fail.txt)


# stress test the compiler, by simulating user's input
# compiler should not crash. Exiting with an exception is OK
# The script should always succeed (otherwise the compiler caught a signal)
add_test(simulate-basic-arithmetic ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/basic-arithmetic.txt)
add_test(simulate-advanced-arithmetic ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/advanced-arithmetic.txt)
add_test(simulate-binary-op ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/binary-op.txt)
add_test(simulate-shift-op ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-op.txt)
add_test(simulate-compound-assignment ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/compound-assignments.txt)
add_test(simulate-binary-assignment ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/binary-assignments.txt)
add_test(simulate-shift-assignment ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-assignments.txt)
add_test(simulate-multiple-logic-op ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/multiple-logic-op.txt)
add_test(simulate-unicode ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/unicode.txt)
add_test(simulate-optimisation-binary-not ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/optimisation-binary-not.txt)
add_test(simulate-for-loop ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/for-loop.txt)
add_test(simulate-while-loop ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/while-loop.txt)
add_test(simulate-when-conditional ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/when-conditional.txt)
add_test(simulate-comments ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/comments.txt)
add_test(simulate-subroutine ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/subroutine.txt)

add_test(simulate-division-by-zero-dyn ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/division-by-zero-dyn.txt)
add_test(simulate-division-by-zero-static ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/division-by-zero-static.txt)
add_test(simulate-chained-conditional ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/chained-conditional.txt)
add_test(simulate-implicit-conditional ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/implicit-conditional.txt)
add_test(simulate-array-access-out-of-bounds-dyn-over ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-dyn-over.txt)
add_test(simulate-array-access-out-of-bounds-dyn-under ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-dyn-under.txt)
add_test(simulate-array-access-out-of-bounds-static-over ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-static-over.txt)
add_test(simulate-array-access-out-of-bounds-static-under ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/array-access-out-of-bounds-static-under.txt)
add_test(simulate-assigning-bool ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/assigning-bool.txt)
add_test(simulate-inconsistent-input1 ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/inconsistent-input1.txt)
add_test(simulate-inconsistent-input2 ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/inconsistent-input2.txt)
add_test(simulate-inconsistent-input3 ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/inconsistent-input3.txt)
add_test(simulate-inconsistent-input4 ${CMAKE_CURRENT_SOURCE_DIR}/simulateuser.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/inconsistent-input4.txt)

# use zzuf to fuzzy the input script deterministically
# compiler should not crash
find_program(zzuf_FOUND zzuf)
if (zzuf_FOUND)
	message("-- zzuf executable found: " ${zzuf_FOUND})
	add_test(fuzzy-basic-arithmetic ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/basic-arithmetic.txt ${fuzzy_loop})
	add_test(fuzzy-advanced-arithmetic ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/advanced-arithmetic.txt ${fuzzy_loop})
	add_test(fuzzy-binary-op ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/binary-op.txt ${fuzzy_loop})
	add_test(fuzzy-shift-op ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-op.txt ${fuzzy_loop})
	add_test(fuzzy-compound-assignment ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/compound-assignments.txt ${fuzzy_loop})
	add_test(fuzzy-binary-assignment ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/binary-assignments.txt ${fuzzy_loop})
	add_test(fuzzy-shift-assignment ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/shift-assignments.txt ${fuzzy_loop})
	add_test(fuzzy-multiple-logic-op ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/multiple-logic-op.txt ${fuzzy_loop})
	add_test(fuzzy-unicode ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/unicode.txt ${fuzzy_loop})
	add_test(fuzzy-optimisation-binary-not ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/optimisation-binary-not.txt ${fuzzy_loop})
	add_test(fuzzy-for-loop ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/for-loop.txt ${fuzzy_loop})
	add_test(fuzzy-while-loop ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/while-loop.txt ${fuzzy_loop})
	add_test(fuzzy-when-conditional ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/when-conditional.txt ${fuzzy_loop})
	add_test(fuzzy-comments ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/comments.txt ${fuzzy_loop})
	add_test(fuzzy-subroutine ${CMAKE_CURRENT_SOURCE_DIR}/fuzzyinput.py ${CMAKE_CURRENT_BINARY_DIR}/asebatest ${CMAKE_CURRENT_SOURCE_DIR}/data/subroutine.txt ${fuzzy_loop})
else (zzuf_FOUND)
	message("-- zzuf executable not found! Disabling fuzzy tests")
endif (zzuf_FOUND)
