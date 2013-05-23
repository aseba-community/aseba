find_package(Qt4)

if (QT4_FOUND)
	set(QT_USE_QTMAIN ON)
	set(QT_USE_QTCONCURRENT ON)
	include(${QT_USE_FILE})

	# add a console for debug messages under Windows
	if (WIN32)
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-Wl,-subsystem,console ${CMAKE_EXE_LINKER_FLAGS}")
	endif (WIN32)
	
	set(thymioupdater_SRCS
		ThymioUpdater.cpp)
	qt4_wrap_cpp(thymioupdater_MOCS
		ThymioUpdater.h
	)
	
	qt4_add_resources(resfiles thymioupdater.qrc)
	
	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
	
	add_executable(thymioupdater WIN32 ${thymioupdater_SRCS} ${thymioupdater_MOCS} ${resfiles} ${CMAKE_BINARY_DIR}/version.h)
	if (HAS_DYN_VERSION)
		add_dependencies(thymioupdater versionheader)
	endif (HAS_DYN_VERSION)

	target_link_libraries(thymioupdater ${QT_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

	install(TARGETS thymioupdater RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif (QT4_FOUND)
