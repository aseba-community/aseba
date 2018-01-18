if (Qt5Core_FOUND AND Qt5DBus_FOUND AND Qt5Xml_FOUND)
	set(CMAKE_AUTOMOC ON)
	
	set(medulla_SRCS medulla.cpp)
	
	add_executable(asebamedulla ${medulla_SRCS})
	
	target_link_libraries(asebamedulla asebacompiler Qt5::Core Qt5::DBus Qt5::Xml ${ASEBA_CORE_LIBRARIES})

	install(TARGETS asebamedulla RUNTIME
		DESTINATION bin
	)
endif ()
