find_package(Qt4)

if (QT4_FOUND AND QT_QTDBUS_FOUND)
	set(QT_USE_QTDBUS ON)
	set(QT_DONT_USE_QTGUI ON)
	include(${QT_USE_FILE})
	
	set(medulla_SRCS
		medulla.cpp
	)
	
	qt4_automoc(${medulla_SRCS})
	
	include_directories (${CMAKE_BINARY_DIR}/medulla)
	
	add_executable(asebamedulla ${medulla_SRCS})
	
	target_link_libraries(asebamedulla asebacore ${QT_LIBRARIES} ${EXTRA_LIBS})

	install(TARGETS asebamedulla RUNTIME
		DESTINATION bin
	)
endif (QT4_FOUND AND QT_QTDBUS_FOUND)