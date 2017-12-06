if (QT4_FOUND AND QT_QTWEBKIT_FOUND)
	set(QT_USE_QTSVG ON)
	include(${QT_USE_FILE})
	include_directories(${CMAKE_BINARY_DIR})
	
	if (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
		include_directories (${QWT_INCLUDE_DIR})
		add_definitions(-DHAVE_QWT)
	endif (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
	if (ENKI_FOUND)
		add_definitions(-DHAVE_ENKI)
	endif (ENKI_FOUND)
	
	add_library(asebaqtabout AboutDialog.cpp)
	if (HAS_DYN_VERSION)
		add_dependencies(asebaqtabout versionheader)
	endif (HAS_DYN_VERSION)
	set_target_properties(asebaqtabout PROPERTIES VERSION ${LIB_VERSION_STRING})

	install(TARGETS asebaqtabout
		LIBRARY DESTINATION ${LIB_INSTALL_DIR}
		ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
	)
endif ()
