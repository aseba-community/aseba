if(EXISTS ${PROJECT_SOURCE_DIR}/enki/CMakeLists.txt)
	add_subdirectory(${PROJECT_SOURCE_DIR}/enki)
	message("Using Enki from ${PROJECT_SOURCE_DIR}/enki")
	set(ENKI_FOUND 1)
else()
	find_package(enki REQUIRED)
	add_library(enki INTERFACE)
	target_include_directories(enki INTERFACE ${enki_INCLUDE_DIR} ${enki_INCLUDE_DIRS})
	target_link_libraries(enki INTERFACE ${enki_LIBRARY})

	find_package(Qt5Widgets)
	find_package(Qt5OpenGL)
	include(FindOpenGL)

	if (Qt5Widgets_FOUND AND Qt5OpenGL_FOUND AND OPENGL_FOUND)
		add_library(enkiviewer INTERFACE)
		target_include_directories(enkiviewer INTERFACE ${enki_INCLUDE_DIR} ${enki_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})
		target_link_libraries(enkiviewer INTERFACE ${enki_VIEWER_LIBRARY} enki Qt5::OpenGL Qt5::Widgets Qt5::Gui Qt5::Core ${OPENGL_LIBRARIES})
	endif()

	set(ENKI_FOUND 1)

endif()
