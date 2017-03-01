# because of jasmine-node, we depend on the executable being named "node", "nodejs" is not working
find_program (NODE_EXECUTABLE NAMES node
	HINTS $ENV{NODE_DIR}
	PATH_SUFFIXES bin
	DOC "Node.js interpreter"
)
find_program(NPM_EXECUTABLE npm
	HINTS $ENV{NODE_DIR}
	PATH_SUFFIXES bin
	DOC "Node.js package manager"
)

if (NODE_EXECUTABLE AND NPM_EXECUTABLE)
	message(STATUS "Node.js and npm found, installing jasmine and frisby")
	# try to install jasmine-node and frisby
	execute_process(COMMAND ${NPM_EXECUTABLE} install jasmine-node WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} RESULT_VARIABLE JASMINE_CHECK_RET OUTPUT_QUIET ERROR_QUIET)
	execute_process(COMMAND ${NPM_EXECUTABLE} install frisby WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} RESULT_VARIABLE FRISBY_CHECK_RET OUTPUT_QUIET ERROR_QUIET)
	if (JASMINE_CHECK_RET EQUAL 0 AND FRISBY_CHECK_RET EQUAL 0)
		message(STATUS "Jasmine-node and Frisby installed successfully, enabling switch e2e tests")
		
		add_custom_command(
			OUTPUT run-e2e-switch.sh
			COMMAND ${CMAKE_COMMAND}
				-D in=${CMAKE_CURRENT_SOURCE_DIR}/run-e2e-switch.sh.in
				-D out=${CMAKE_CURRENT_BINARY_DIR}/run-e2e-switch.sh
				-D ASEBASWITCH=$<TARGET_FILE:asebaswitch>
				-D ASEBADUMMYNODE=$<TARGET_FILE:asebadummynode>
				-P ${CMAKE_CURRENT_SOURCE_DIR}/../patch-script.cmake
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/run-e2e-switch.sh.in
		)
		add_custom_target(create-run-e2e-switch ALL DEPENDS run-e2e-switch.sh)
		
		# copy all test files
		file(GLOB jsFiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.js")
		foreach(jsFile ${jsFiles})
			configure_file(${jsFile} ${CMAKE_CURRENT_BINARY_DIR}/${jsFile} COPYONLY)
		endforeach()
		
		add_test(switch-e2e sh run-e2e-switch.sh)
	else()
		message(STATUS "Jasmine-node or Frisby NOT installed, disabling switch e2e tests")
	endif ()
else ()
	message(STATUS "Node.js or npm NOT found, disabling switch e2e tests")
endif ()
