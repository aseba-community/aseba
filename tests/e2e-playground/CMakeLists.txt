# End-to-end testing of Aseba Playground multi-node networks


# Test multi-node networks

# 1. using processes
add_test(NAME e2e-playground-1-processes
	COMMAND bash run-e2e-playground.sh test-1-processes.playground)
set_tests_properties(e2e-playground-1-processes PROPERTIES TIMEOUT 90)

# 2. using proposed <networks> tag: expected to fail
add_test(NAME e2e-playground-2-networks
	COMMAND bash run-e2e-playground.sh test-2-networks.playground)
set_tests_properties(e2e-playground-2-networks PROPERTIES TIMEOUT 90 WILL_FAIL true)


# Create test fixtures

# 1. test program with correct target utilities
add_custom_command(
	OUTPUT run-e2e-playground.sh
	COMMAND ${CMAKE_COMMAND}
		-D in=${CMAKE_CURRENT_SOURCE_DIR}/run-e2e-playground.sh.in
		-D out=${CMAKE_CURRENT_BINARY_DIR}/run-e2e-playground.sh
		-D ASEBAPLAYGROUND=$<TARGET_FILE:asebaplayground> 
		-D ASEBADUMP=$<TARGET_FILE:asebadump> 
		-P ${CMAKE_CURRENT_SOURCE_DIR}/../patch-script.cmake
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/run-e2e-playground.sh.in
)
add_custom_target(create-run-e2e-playground ALL DEPENDS run-e2e-playground.sh)

# 2. patch test-1-processes.playground with correct target utilities
add_custom_command(
	OUTPUT test-1-processes.playground
	COMMAND ${CMAKE_COMMAND}
		-D in=${CMAKE_CURRENT_SOURCE_DIR}/test-1-processes.playground.in
		-D out=${CMAKE_CURRENT_BINARY_DIR}/test-1-processes.playground
		-D ASEBAMASSLOADER=$<TARGET_FILE:asebamassloader> 
		-D ASEBASWITCH=$<TARGET_FILE:asebaswitch> 
		-D ASEBAHTTP=$<TARGET_FILE:asebahttp> 
		-P ${CMAKE_CURRENT_SOURCE_DIR}/../patch-script.cmake
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test-1-processes.playground.in
)
add_custom_target(create-test-1-processes ALL DEPENDS test-1-processes.playground)

# 3. other assets are copied
configure_file(test-2-networks.playground ${CMAKE_CURRENT_BINARY_DIR}/test-2-networks.playground COPYONLY)
configure_file(beacon-solo.aesl ${CMAKE_CURRENT_BINARY_DIR}/beacon-solo.aesl COPYONLY)
configure_file(beacon-net.aesl ${CMAKE_CURRENT_BINARY_DIR}/beacon-net.aesl COPYONLY)
configure_file(README.md ${CMAKE_CURRENT_BINARY_DIR}/README.md COPYONLY)

