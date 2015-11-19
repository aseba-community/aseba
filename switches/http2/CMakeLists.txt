# need libxml2 to read aesl files
find_package(LibXml2)
if(LIBXML2_FOUND)
	include_directories(${LIBXML2_INCLUDE_DIR})

	set(http2_SRCS
		AeslProgram.cpp
		HttpDashelTarget.cpp
		HttpInterface.cpp
		HttpInterfaceHandlers.cpp
		HttpRequest.cpp
		HttpResponse.cpp
		main.cpp
	)
	
	add_executable(asebahttp2 ${http2_SRCS})
	
	target_link_libraries(asebahttp2 asebacompiler asebacommon ${LIBXML2_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

	install(TARGETS asebahttp2 RUNTIME
		DESTINATION bin
	)

	add_library(asebahttp2hub ${http2_SRCS})
	set_target_properties(asebahttp2hub PROPERTIES VERSION ${LIB_VERSION_STRING} SOVERSION ${LIB_VERSION_MAJOR})

	install(TARGETS asebahttp2hub
			LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
			ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
	)

	set(ASEBACORE_HDR_HTTP2
		AeslProgram.h
		HttpDashelTarget.h
		HttpHandler.h
		HttpInterface.h
		HttpInterfaceHandlers.h
		HttpRequest.h
		HttpResponse.h
	)
	
	install(FILES ${ASEBACORE_HDR_HTTP2}
		DESTINATION include/aseba/switches/http2
	)
endif(LIBXML2_FOUND)
