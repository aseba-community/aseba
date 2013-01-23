find_package(Qt4)

if (QT4_FOUND)
	set(QT_USE_QTMAIN ON)
	set(QT_USE_QTCONCURRENT ON)
	include(${QT_USE_FILE})

	# add a console for debug messages under Windows
	if (WIN32)
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-Wl,-subsystem,console ${CMAKE_EXE_LINKER_FLAGS}")
	endif (WIN32)
	
	set(thymioflasher_SRCS
		ThymioFlasher.cpp)
	
	qt4_automoc(${thymioflasher_SRCS})
	
	qt4_add_resources(resfiles thymioflasher.qrc)
	
	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
	
	add_executable(thymioflasher WIN32 ${thymioflasher_SRCS} ${resfiles} ${CMAKE_BINARY_DIR}/version.h)
	if (HAS_DYN_VERSION)
		add_dependencies(thymioflasher versionheader)
	endif (HAS_DYN_VERSION)

	target_link_libraries(thymioflasher asebacore ${QT_LIBRARIES} ${EXTRA_LIBS})

	install(TARGETS thymioflasher RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif (QT4_FOUND)
