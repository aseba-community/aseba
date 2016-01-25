if (QT4_FOUND)
	set(QT_USE_QTNETWORK ON)
	set(QT_USE_QTMAIN ON)
	include(${QT_USE_FILE})
	
	set(aseba-qt-gui_SRCS
		main.cpp
		dashelinterface.cpp
		mainwindow.cpp
	)
	
	qt4_automoc(${aseba-qt-gui_SRCS})

	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

	add_executable(aseba-qt-gui WIN32 ${aseba-qt-gui_SRCS})
	target_link_libraries(aseba-qt-gui ${ASEBA_CORE_LIBRARIES} ${QT_LIBRARIES} ${EXTRA_LIBS})
endif (QT4_FOUND)
