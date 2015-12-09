if (QT4_FOUND)
	set(QT_USE_QTMAIN ON)
	include(${QT_USE_FILE})
	
	set(thymiownetconfig_SRCS
		ThymioWNetConfig.cpp)
	qt4_wrap_cpp(thymiownetconfig_MOCS
		ThymioWNetConfig.h
	)
	
	qt4_add_resources(resfiles thymiownetconfig.qrc)
	
	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
	
	add_executable(thymiownetconfig WIN32 ${thymiownetconfig_SRCS} ${thymiownetconfig_MOCS} ${resfiles} ${CMAKE_BINARY_DIR}/version.h)
	if (HAS_DYN_VERSION)
		add_dependencies(thymiownetconfig versionheader)
	endif (HAS_DYN_VERSION)

	target_link_libraries(thymiownetconfig ${QT_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

	install(TARGETS thymiownetconfig RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif (QT4_FOUND)
