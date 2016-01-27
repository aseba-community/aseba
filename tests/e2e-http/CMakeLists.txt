add_custom_command(
    OUTPUT run-e2e-http.sh
    COMMAND perl ARGS ${CMAKE_CURRENT_SOURCE_DIR}/patch-script.perl 
    	    ${CMAKE_CURRENT_SOURCE_DIR}/run-e2e-http.sh.in 
	    ${CMAKE_CURRENT_BINARY_DIR}/run-e2e-http.sh 
	    $<TARGET_FILE:asebahttp> 
	    $<TARGET_FILE:asebadummynode> 
	    $<TARGET_FILE:asebaswitch> 
	    $<TARGET_FILE:asebamassloader>
    COMMENT "Copy then substitute" $<TARGET_FILE:asebahttp>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/run-e2e-http.sh.in
)
add_custom_target(
    create_run_script ALL
    DEPENDS run-e2e-http.sh
    COMMENT "Running create_run_script"
)

find_program( HAVE_NPM npm )

if (HAVE_NPM)
    add_custom_target(
	install_jasmine ALL
	COMMAND npm install
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Installing jasmine"
    )
endif (HAVE_NPM)

configure_file(1-http_spec.js ${CMAKE_CURRENT_BINARY_DIR}/1-http_spec.js COPYONLY)
configure_file(2-interaction_spec.js ${CMAKE_CURRENT_BINARY_DIR}/2-interaction_spec.js COPYONLY)
configure_file(3-remap_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3-remap_spec.js COPYONLY)
configure_file(package.json ${CMAKE_CURRENT_BINARY_DIR}/package.json COPYONLY)
configure_file(ping0123.aesl ${CMAKE_CURRENT_BINARY_DIR}/ping0123.aesl COPYONLY)

add_test(NAME e2e-http COMMAND bash run-e2e-http.sh)
