find_package(Qt4)

if (QT4_FOUND)
	set(QT_USE_QTNETWORK ON)
	set(QT_USE_QTMAIN ON)
	include(${QT_USE_FILE})
	
	set(qt-gui_SRCS
		main.cpp
		dashelinterface.cpp
		mainwindow.cpp
	)
	
	qt4_automoc(${qt-gui_SRCS})

	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

	add_executable(qt-gui WIN32 ${qt-gui_SRCS})
	target_link_libraries(qt-gui asebacore ${QT_LIBRARIES} ${EXTRA_LIBS})
	install(TARGETS qt-gui RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
endif (QT4_FOUND)
