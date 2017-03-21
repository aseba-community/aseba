add_subdirectory(simulator)
add_subdirectory(vm)
add_subdirectory(compiler)

# test asebahttp
# find_package(LibXml2)
# if (LIBXML2_FOUND)
#         include_directories(${LIBXML2_INCLUDE_DIR})
# 	include_directories(externals/Catch/include ${COMMON_INCLUDES})
# 	add_executable(test-asebahttp test-http.cpp)
# 	target_link_libraries(test-asebahttp asebahttphub asebacompiler ${LIBXML2_LIBRARIES} ${ASEBA_CORE_LIBRARIES})
# 	configure_file(testdata-HttpRequest.txt ${CMAKE_CURRENT_BINARY_DIR}/testdata-HttpRequest.txt COPYONLY)
# 	configure_file(http-valgrind-macosx.supp ${CMAKE_CURRENT_BINARY_DIR}/http-valgrind-macosx.supp COPYONLY)
# 	# test HTTP requests and JSON parsing
# 
# add_custom_command(
# 	OUTPUT run-test-asebahttp.sh
# 	COMMAND ${CMAKE_COMMAND}
# 		-D in=${CMAKE_CURRENT_SOURCE_DIR}/run-test-asebahttp.sh.in
# 		-D out=${CMAKE_CURRENT_BINARY_DIR}/run-test-asebahttp.sh
# 		-D ASEBADUMMYNODE=$<TARGET_FILE:asebadummynode>
# 		-D TESTASEBAHTTP=$<TARGET_FILE:test-asebahttp>
# 		-P ${CMAKE_CURRENT_SOURCE_DIR}/patch-script.cmake
# 		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
# 		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/run-test-asebahttp.sh.in
# )
# add_custom_target(
# 	create-run-test-asebahttp ALL
# 	DEPENDS run-test-asebahttp.sh
# )
# 	add_test(NAME test-asebahttp COMMAND bash run-test-asebahttp.sh)
# else (LIBXML2_FOUND)
# 	message("-- libXML2 not found! Disabling HTTP switch tests")
# endif (LIBXML2_FOUND)

# End-to-end testing
add_subdirectory(e2e-http)
