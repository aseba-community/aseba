# asebahttp - a switch to bridge HTTP to Aseba
# 2014-12-01 David James Sherman <david dot sherman at inria dot fr>

# need libxml2 to read aesl files
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
	
	add_executable(asebahttp ${http_SRCS} ${http_MOCS})
	
	target_link_libraries(asebahttp asebacompiler asebacommon ${LIBXML2_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

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

	configure_file(dummynode-1-tick.aesl ${CMAKE_CURRENT_BINARY_DIR}/dummynode-1-tick.aesl COPYONLY)
	configure_file(dummynode-1.aesl ${CMAKE_CURRENT_BINARY_DIR}/dummynode-1.aesl COPYONLY)

endif (LIBXML2_FOUND)
