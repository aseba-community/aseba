if (Qt5Widgets_FOUND)

	set(thymiownetconfig_SRCS
		ThymioWNetConfig.cpp)
 	qt5_wrap_cpp(thymiownetconfig_MOCS
 		ThymioWNetConfig.h
 	)

	file(GLOB TRANSLATIONS_SOURCES "thymiownetconfig*.ts")
	set_source_files_properties(${TRANSLATIONS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}")
	qt5_add_translation(TRANSLATIONS ${TRANSLATIONS_SOURCES} )
	qt5_add_resources(resfiles thymiownetconfig.qrc)

	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

	add_executable(thymiownetconfig WIN32 ${thymiownetconfig_SRCS} ${thymiownetconfig_MOCS} ${resfiles})
	if (HAS_DYN_VERSION)
		add_dependencies(thymiownetconfig versionheader)
	endif (HAS_DYN_VERSION)

	target_link_libraries(thymiownetconfig Qt5::Widgets asebacommon asebadashelplugins)

	install_qt_app(thymiownetconfig)
	codesign(thymiownetconfig)

endif ()

if (UNIX)
	add_executable(thymiownetconfig-cli thymiownetconfig-cli.c)
	target_link_libraries(thymiownetconfig-cli aseba_conf asebacommon)
	install(TARGETS thymiownetconfig-cli RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
endif (UNIX)
