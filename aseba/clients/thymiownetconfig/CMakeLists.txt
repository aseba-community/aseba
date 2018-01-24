if (Qt5Widgets_FOUND)
	
	set(thymiownetconfig_SRCS
		ThymioWNetConfig.cpp)
 	qt5_wrap_cpp(thymiownetconfig_MOCS
 		ThymioWNetConfig.h
 	)
	
	qt5_add_resources(resfiles thymiownetconfig.qrc)
	
	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
	
	add_executable(thymiownetconfig WIN32 ${thymiownetconfig_SRCS} ${thymiownetconfig_MOCS} ${resfiles} ${CMAKE_BINARY_DIR}/version.h)
	if (HAS_DYN_VERSION)
		add_dependencies(thymiownetconfig versionheader)
	endif (HAS_DYN_VERSION)

	target_link_libraries(thymiownetconfig Qt5::Widgets ${ASEBA_CORE_LIBRARIES})

	install(TARGETS thymiownetconfig RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif ()

if (UNIX)
	add_executable(thymiownetconfig-cli thymiownetconfig-cli.c)
	install(TARGETS thymiownetconfig-cli RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
endif (UNIX)
