# libxml2
find_package(LibXml2)
if (LIBXML2_FOUND)
	
        include_directories(${LIBXML2_INCLUDE_DIR})

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
	
	target_link_libraries(asebahttp asebacompiler ${LIBXML2_LIBRARIES} ${ASEBA_CORE_LIBRARIES})
	target_link_libraries(test-asebahttp asebacompiler ${LIBXML2_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

	configure_file(testdata-HttpRequest.txt ${CMAKE_CURRENT_BINARY_DIR}/testdata-HttpRequest.txt COPYONLY)
	configure_file(do_valgrind_macosx.sh ${CMAKE_CURRENT_BINARY_DIR}/do_valgrind_macosx.sh COPYONLY)
	configure_file(valgrind-macosx.supp ${CMAKE_CURRENT_BINARY_DIR}/valgrind-macosx.supp COPYONLY)
	configure_file(dummynode-1-tick.aesl ${CMAKE_CURRENT_BINARY_DIR}/dummynode-1-tick.aesl COPYONLY)
	configure_file(dummynode-1.aesl ${CMAKE_CURRENT_BINARY_DIR}/dummynode-1.aesl COPYONLY)
	configure_file(example-nodered-simple.json ${CMAKE_CURRENT_BINARY_DIR}/example-nodered-simple.json COPYONLY)

	add_test(NAME test-asebahttp
	         COMMAND test-asebahttp)

	install(TARGETS asebahttp RUNTIME
		DESTINATION bin
	)

add_library(asebahttphub ${http_SRCS})
set_target_properties(asebahttphub PROPERTIES VERSION ${LIB_VERSION_STRING} 
                                            SOVERSION ${LIB_VERSION_MAJOR})

install(TARGETS asebahttphub
                LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
)

set (ASEBACORE_HDR_HTTP
        http.h
)
install(FILES ${ASEBACORE_HDR_HTTP}
        DESTINATION include/aseba/switches/http
)




endif (LIBXML2_FOUND)
