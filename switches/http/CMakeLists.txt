# libjson
find_package(LibJson)
# libxml2
find_package(LibXml2)
if (LIBXML2_FOUND AND LIBJSON_FOUND)
	
        include_directories(${LIBXML2_INCLUDE_DIR} ${LIBJSON_INCLUDE_DIR})

	set(http_SRCS
		http.cpp
		main.cpp
	)
	set(http_MOCS
		http.h
	)
	set(test_http_SRCS
		http.cpp
		test-http.cpp
	)
	
	add_executable(asebahttp ${http_SRCS} ${http_MOCS})
	add_executable(test-asebahttp ${test_http_SRCS} ${http_MOCS})
	
	target_link_libraries(asebahttp asebacompiler ${LIBXML2_LIBRARIES} ${LIBJSON_LIBRARY} ${ASEBA_CORE_LIBRARIES})
	target_link_libraries(test-asebahttp asebacompiler ${LIBXML2_LIBRARIES} ${LIBJSON_LIBRARY} ${ASEBA_CORE_LIBRARIES})

	configure_file(testdata-HttpRequest.txt ${CMAKE_CURRENT_BINARY_DIR}/testdata-HttpRequest.txt COPYONLY)

	add_test(NAME test-asebahttp
	         COMMAND test-asebahttp)

	install(TARGETS asebahttp RUNTIME
		DESTINATION bin
	)
endif (LIBXML2_FOUND AND LIBJSON_FOUND)
