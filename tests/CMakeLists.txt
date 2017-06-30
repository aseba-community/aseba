add_subdirectory(msg)
add_subdirectory(compiler)
add_subdirectory(vm)
add_subdirectory(simulator)
add_subdirectory(e2e-http)


# test asebahttp
# FIXME: port to asebahttp2
#find_package(LibXml2)
#if (LIBXML2_FOUND)
#	include_directories(${LIBXML2_INCLUDE_DIR})
#	include_directories(externals/Catch/include ${COMMON_INCLUDES})
#	add_executable(test-asebahttp test-http.cpp)
#	if (CMAKE_VERSION VERSION_EQUAL "2.8.12" OR CMAKE_VERSION VERSION_GREATER "2.8.12")
#		# Support compilers like GCC 5 that don't have C++11 support enabled by default.
#		target_compile_definitions(test-asebahttp PRIVATE -DCATCH_CONFIG_NO_CPP11)
#	endif ()
#	target_link_libraries(test-asebahttp asebahttphub asebacompiler ${LIBXML2_LIBRARIES} ${ASEBA_CORE_LIBRARIES})
#	configure_file(testdata-HttpRequest.txt ${CMAKE_CURRENT_BINARY_DIR}/testdata-HttpRequest.txt COPYONLY)
#	configure_file(http-valgrind-macosx.supp ${CMAKE_CURRENT_BINARY_DIR}/http-valgrind-macosx.supp COPYONLY)
#	# test HTTP requests and JSON parsing
#	configure_file(run-test-asebahttp.sh ${CMAKE_CURRENT_BINARY_DIR}/run-test-asebahttp.sh COPYONLY)
#	add_test(NAME test-asebahttp COMMAND bash run-test-asebahttp.sh)
#else (LIBXML2_FOUND)
#	message(WARNING "libXML2 not found! Disabling HTTP switch tests")
#endif (LIBXML2_FOUND)
