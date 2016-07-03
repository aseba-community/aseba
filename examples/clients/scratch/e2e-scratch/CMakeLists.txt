add_custom_command(
    OUTPUT run-e2e-scratch.sh
    COMMAND perl ARGS ${CMAKE_CURRENT_SOURCE_DIR}/patch-script.perl 
    	    ${CMAKE_CURRENT_SOURCE_DIR}/run-e2e-scratch.sh.in 
	    ${CMAKE_CURRENT_BINARY_DIR}/run-e2e-scratch.sh 
	    ASEBAHTTP=$<TARGET_FILE:asebahttp> 
	    ASEBADUMMYNODE=$<TARGET_FILE:asebadummynode> 
	    ASEBASWITCH=$<TARGET_FILE:asebaswitch> 
	    ASEBAMASSLOADER=$<TARGET_FILE:asebamassloader>
	    ASEBASCRATCH=$<TARGET_FILE:asebascratch>
	    DASHELPORTLIST=${dashel_DIR}/portlist
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/run-e2e-scratch.sh.in
)
add_custom_target(
    create-run-e2e-scratch ALL
    DEPENDS run-e2e-scratch.sh
)

add_custom_command(
    OUTPUT run-e2e-scratch-switch.sh
    COMMAND perl ARGS ${CMAKE_CURRENT_SOURCE_DIR}/patch-script.perl 
    	    ${CMAKE_CURRENT_SOURCE_DIR}/run-e2e-scratch-switch.sh.in 
	    ${CMAKE_CURRENT_BINARY_DIR}/run-e2e-scratch-switch.sh 
	    ASEBAHTTP=$<TARGET_FILE:asebahttp> 
	    ASEBADUMMYNODE=$<TARGET_FILE:asebadummynode> 
	    ASEBASWITCH=$<TARGET_FILE:asebaswitch> 
	    ASEBAMASSLOADER=$<TARGET_FILE:asebamassloader>
	    ASEBASCRATCH=$<TARGET_FILE:asebascratch>
	    DASHELPORTLIST=${dashel_DIR}/portlist
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/run-e2e-scratch-switch.sh.in
)
add_custom_target(
    create-run-e2e-scratch-switch ALL
    DEPENDS run-e2e-scratch-switch.sh
)

find_program( HAVE_NPM npm )

if (HAVE_NPM)
    if (WIN32)
        set(NPM_INSTALL cmd //c npm install)
    else (WIN32)
        set(NPM_INSTALL npm install)
    endif (WIN32)
    add_custom_target(
	install_jasmine_for_asebascratch ALL
	COMMAND ${NPM_INSTALL}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Installing jasmine"
    )
endif (HAVE_NPM)

configure_file(3000-1-http_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3000-1-http_spec.js COPYONLY)
configure_file(3000-2-interaction_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3000-2-interaction_spec.js COPYONLY)
configure_file(3001-1-http_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3001-1-http_spec.js COPYONLY)
configure_file(3001-2-remap_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3001-2-remap_spec.js COPYONLY)
configure_file(3001-3-load-aesl_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3001-3-load-aesl_spec.js COPYONLY)
configure_file(3002-1-thymio-II_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3002-1-thymio-II_spec.js COPYONLY)
configure_file(3002-2-scratch_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3002-2-scratch_spec.js COPYONLY)
configure_file(3002-3-scratch-queue_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3002-3-scratch-queue_spec.js COPYONLY)
configure_file(3004-1-switch_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3004-1-switch_spec.js COPYONLY)
configure_file(3005-1-switch_spec.js ${CMAKE_CURRENT_BINARY_DIR}/3005-1-switch_spec.js COPYONLY)
configure_file(package.json ${CMAKE_CURRENT_BINARY_DIR}/package.json COPYONLY)
configure_file(ping0123.aesl ${CMAKE_CURRENT_BINARY_DIR}/ping0123.aesl COPYONLY)
configure_file(dice.aesl ${CMAKE_CURRENT_BINARY_DIR}/dice.aesl COPYONLY)
configure_file(thymio_motion.aesl ${CMAKE_CURRENT_BINARY_DIR}/thymio_motion.aesl COPYONLY)
configure_file(README.md ${CMAKE_CURRENT_BINARY_DIR}/README.md COPYONLY)

add_test(NAME e2e-scratch COMMAND bash run-e2e-scratch.sh )
add_test(NAME e2e-scratch-switch COMMAND bash run-e2e-scratch-switch.sh )

find_program( HAVE_SIEGE siege )

if (HAVE_SIEGE)
	configure_file(run-generate-messages.perl ${CMAKE_CURRENT_BINARY_DIR}/run-generate-messages.perl COPYONLY)
	configure_file(run-record-asebascratch.sh ${CMAKE_CURRENT_BINARY_DIR}/run-record-asebascratch.sh COPYONLY)
	configure_file(run-record-mockasebascratch.sh ${CMAKE_CURRENT_BINARY_DIR}/run-record-mockasebascratch.sh COPYONLY)
endif (HAVE_SIEGE)

# mock-asebascratch uses std::thread
if (CMAKE_VERSION VERSION_LESS "3.1")
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
	endif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
else (CMAKE_VERSION VERSION_LESS "3.1")
	set (CMAKE_CXX_STANDARD 11)
endif (CMAKE_VERSION VERSION_LESS "3.1")

add_executable( mock-asebascratch mock-asebascratch.cpp )
target_link_libraries( mock-asebascratch ${ASEBA_CORE_LIBRARIES} )
