if (QT4_FOUND AND QT_QTDBUS_FOUND)
	set(QT_USE_QTDBUS ON)
	set(QT_USE_QTXML ON)
	set(QT_USE_QTMAIN ON)
	set(QT_DONT_USE_QTGUI ON)
	include(${QT_USE_FILE})
	
	set(medulla_SRCS medulla.cpp)
	qt4_wrap_cpp(medulla_MOCS medulla.h)
	
	add_executable(asebamedulla ${medulla_SRCS} ${medulla_MOCS})
	
	target_link_libraries(asebamedulla asebacompiler ${QT_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

	install(TARGETS asebamedulla RUNTIME
		DESTINATION bin
	)
endif (QT4_FOUND AND QT_QTDBUS_FOUND)
