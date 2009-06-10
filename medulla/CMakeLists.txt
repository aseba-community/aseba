find_package(Qt4)

if (QT4_FOUND)
	include(${QT_USE_FILE})

	set(medulla_SRCS
		medulla.cpp
	)
	
	qt4_automoc(${medulla_SRCS})
	
	include_directories (${QT_INCLUDE_DIR} ${QT_QTXML_INCLUDE_DIR} ${QT_QTDBUS_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/medulla)
	
	add_executable(asebamedulla ${medulla_SRCS})
	
	target_link_libraries(asebamedulla asebacore ${QT_LIBRARIES} ${QT_QTDBUS_LIBRARIES} ${EXTRA_LIBS})

	install(TARGETS asebamedulla RUNTIME
		DESTINATION bin
	)
endif (QT4_FOUND)