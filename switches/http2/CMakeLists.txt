	set(switch_SRCS
		http/Json.cpp
		http/HttpStatus.cpp
		http/HttpMethod.cpp
		http/HttpProtocol.cpp
		http/HttpRequest.cpp
		http/HttpResponse.cpp
		http/HttpDispatcher.cpp
		http/EventsHandlers.cpp
		http/ConstantsHandlers.cpp
		http/EventsHandlers.cpp
		http/NodesHandlers.cpp
		http/DebugsHandlers.cpp
		Globals.cpp
		DashelUtils.cpp
		Arguments.cpp
		Switch.cpp
		main.cpp
	)
	
	add_executable(asebaswitch ${switch_SRCS})
	
	target_link_libraries(asebaswitch asebacompiler asebacommon ${ASEBA_CORE_LIBRARIES})
	
	install(TARGETS asebaswitch RUNTIME
		DESTINATION bin
	)
