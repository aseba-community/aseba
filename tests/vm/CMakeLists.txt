# test the number of native functions in the vm
add_executable(aseba-test-natives-count
	aseba-test-natives-count.cpp
)
target_link_libraries(aseba-test-natives-count asebacompiler asebavm asebavmdummycallbacks ${ASEBA_CORE_LIBRARIES})
add_test(natives-count ${EXECUTABLE_OUTPUT_PATH}/aseba-test-natives-count)

# test the deque native functions
add_test(NAME deque-empty COMMAND asebatest --memcmp
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-empty.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/deque-empty.txt)
add_test(NAME deque-getset COMMAND asebatest --memcmp
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-getset.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/deque-getset.txt)
add_test(NAME deque-insert COMMAND asebatest --memcmp
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-insert.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/deque-insert.txt)
add_test(NAME deque-remove COMMAND asebatest --memcmp
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-remove.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/deque-remove.txt)
add_test(NAME deque-erase-wrap COMMAND asebatest --memcmp
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-erase-wrap.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/deque-erase-wrap.txt)
add_test(NAME deque-insert-wrap COMMAND asebatest --memcmp
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-insert-wrap.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/deque-insert-wrap.txt)
add_test(NAME deque-tuples COMMAND asebatest --memcmp
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-tuples.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/deque-tuples.txt)
add_test(NAME deque-pushpop COMMAND asebatest --memcmp
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-pushpop.dump ${CMAKE_CURRENT_SOURCE_DIR}/data/deque-pushpop.txt)

# test exceptions raised by deque native functions
add_test(NAME deque-err-get-under COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-get-under.txt)
add_test(NAME deque-err-get-over COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-get-over.txt)
add_test(NAME deque-err-get-toobig COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-get-toobig.txt)
add_test(NAME deque-err-set-under COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-set-under.txt)
add_test(NAME deque-err-set-over COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-set-over.txt)
add_test(NAME deque-err-set-toobig COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-set-toobig.txt)
add_test(NAME deque-err-insert-under COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-insert-under.txt)
add_test(NAME deque-err-insert-over COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-insert-over.txt)
add_test(NAME deque-err-insert-toobig COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-insert-toobig.txt)
add_test(NAME deque-err-erase-under COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-erase-under.txt)
add_test(NAME deque-err-erase-len-under COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-erase-len-under.txt)
add_test(NAME deque-err-erase-over COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-erase-over.txt)
add_test(NAME deque-err-erase-toobig COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-erase-toobig.txt)
add_test(NAME deque-err-push-toobig COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-push-toobig.txt)
add_test(NAME deque-err-pop-toobig COMMAND asebatest --exec_fail
	${CMAKE_CURRENT_SOURCE_DIR}/data/deque-err-pop-toobig.txt)
