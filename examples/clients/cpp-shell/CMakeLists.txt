# libxml2
find_package(LibXml2)
if (LIBXML2_FOUND)
	include_directories(${LIBXML2_INCLUDE_DIR})

	add_executable(asebashell
		shell.cpp
	)
	target_link_libraries(asebashell asebacompiler ${ASEBA_CORE_LIBRARIES} ${LIBXML2_LIBRARIES})
	install(TARGETS asebashell RUNTIME
		DESTINATION bin
	)
endif (LIBXML2_FOUND)