if (Qt5Widgets_FOUND AND Qt5Concurrent_FOUND AND Qt5Network_FOUND AND Qt5Xml_FOUND)

	# add a console for debug messages under Windows
	if (WIN32)
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-Wl,-subsystem,console ${CMAKE_EXE_LINKER_FLAGS}")
	endif (WIN32)

	set(thymioupgrader_SRCS
		ThymioUpgrader.cpp)
	qt5_wrap_cpp(thymioupgrader_MOCS
		ThymioUpgrader.h
	)

	file(GLOB TRANSLATIONS_SOURCES "thymioupgrader*.ts")
	set_source_files_properties(${TRANSLATIONS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}")
	qt5_add_translation(TRANSLATIONS ${TRANSLATIONS_SOURCES} )
	qt5_add_resources(resfiles thymioupgrader.qrc)

	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

	add_executable(thymioupgrader WIN32 ${thymioupgrader_SRCS} ${thymioupgrader_MOCS} ${resfiles})
	if (HAS_DYN_VERSION)
		add_dependencies(thymioupgrader versionheader)
	endif (HAS_DYN_VERSION)

	target_link_libraries(thymioupgrader Qt5::Concurrent Qt5::Network Qt5::Xml Qt5::Widgets asebacommon asebadashelplugins)

	install_qt_app(thymioupgrader)
	codesign(thymioupgrader)

endif ()
