if(EXISTS ${PROJECT_SOURCE_DIR}/enki/CMakeLists.txt)
	add_subdirectory(${PROJECT_SOURCE_DIR}/enki)
	message("Using Enki from ${PROJECT_SOURCE_DIR}/enki")
else()
	find_package(enki REQUIRED)
	add_library(enki INTERFACE)
	target_include_directories(enki INTERFACE ${enki_INCLUDE_DIRS})
	target_link_libraries(enki INTERFACE ${enki_LIBRARIES})

	add_library(enki_viewer INTERFACE)
	target_link_libraries(enki INTERFACE enki ${enki_VIEWER_LIBRARY} Qt5::OpenGL Qt5::Widgets Qt5::Gui Qt5::Core)
endif()
