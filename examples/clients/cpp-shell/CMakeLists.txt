# libxml2
find_package(LibXml2)
if (LIBXML2_FOUND)
	include_directories(${LIBXML2_INCLUDE_DIR})

	add_executable(asebashell
		shell.cpp
	)
	target_link_libraries(asebashell asebacompiler ${LIBXML2_LIBRARIES} ${ASEBA_CORE_LIBRARIES})
endif (LIBXML2_FOUND)