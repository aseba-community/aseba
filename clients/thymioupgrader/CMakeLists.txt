if (QT4_FOUND)
	set(QT_USE_QTMAIN ON)
	set(QT_USE_QTCONCURRENT ON)
	include(${QT_USE_FILE})

	# add a console for debug messages under Windows
	if (WIN32)
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-Wl,-subsystem,console ${CMAKE_EXE_LINKER_FLAGS}")
	endif (WIN32)
	
	set(thymioupgrader_SRCS
		ThymioUpgrader.cpp)
	qt4_wrap_cpp(thymioupgrader_MOCS
		ThymioUpgrader.h
	)
	
	qt4_add_resources(resfiles thymioupgrader.qrc)
	
	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
	
	add_executable(thymioupgrader WIN32 ${thymioupgrader_SRCS} ${thymioupgrader_MOCS} ${resfiles} ${CMAKE_BINARY_DIR}/version.h)
	if (HAS_DYN_VERSION)
		add_dependencies(thymioupgrader versionheader)
	endif (HAS_DYN_VERSION)

	target_link_libraries(thymioupgrader ${QT_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

	install(TARGETS thymioupgrader RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif (QT4_FOUND)
