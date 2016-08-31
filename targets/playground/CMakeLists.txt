if (QT4_FOUND AND ENKI_FOUND)

	find_package(OpenGL REQUIRED)
	
	set(QT_USE_QTMAIN ON)
	set(QT_USE_QTOPENGL ON)
	set(QT_USE_QTXML ON)
	if (QT_QTDBUS_FOUND AND NOT WIN32)
		# force disable D-Bus on Windows, because it generates compilation
		# errors in interaction with QtOpenGL includes
		set(QT_USE_QTDBUS ON)
		add_definitions("-DHAVE_DBUS")
	endif (QT_QTDBUS_FOUND AND NOT WIN32)
	include(${QT_USE_FILE})
	
	include_directories(${ENKI_INCLUDE_DIR})
	
	set(playground_SRCS
		AsebaGlue.cpp
		Door.cpp
		EPuck.cpp
		EPuck-descriptions.c
		Thymio2.cpp
		Thymio2-natives.cpp
		Thymio2-descriptions.c
		PlaygroundViewer.cpp
		PlaygroundDBusAdaptors.cpp
		playground.cpp
	)
	qt4_wrap_cpp(playground_MOCS
		Thymio2.h
		PlaygroundViewer.h
		PlaygroundDBusAdaptors.h
	)
	
	add_executable(asebaplayground WIN32 ${playground_SRCS} ${playground_MOCS} ${playground_RCC_SRCS})
	
	target_link_libraries(asebaplayground asebavmbuffer asebavm ${ENKI_VIEWER_LIBRARY} ${ENKI_LIBRARY} ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ASEBA_CORE_LIBRARIES} ${EXTRA_LIBS})
	install(TARGETS asebaplayground RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif (QT4_FOUND AND ENKI_FOUND)
