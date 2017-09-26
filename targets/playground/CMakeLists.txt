if (ENKI_FOUND)
	include_directories(${enki_INCLUDE_DIR})
	
	set (ASEBASIM_SRC
		EnkiGlue.cpp
		AsebaGlue.cpp
		DirectAsebaGlue.cpp
		Door.cpp
		EPuck.cpp
		EPuck-descriptions.c
		Thymio2.cpp
		Thymio2-natives.cpp
		Thymio2-descriptions.c
	)

	add_library(asebasim ${ASEBASIM_SRC})
	set_target_properties(asebasim PROPERTIES VERSION ${LIB_VERSION_STRING} 
											SOVERSION ${LIB_VERSION_MAJOR})
	
	install(TARGETS asebasim 
			LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
			ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
	)
	
	if (QT4_FOUND)

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
		
		set(playground_SRCS
			DashelAsebaGlue.cpp
			PlaygroundViewer.cpp
			PlaygroundDBusAdaptors.cpp
			playground.cpp
		)
		qt4_wrap_cpp(playground_MOCS
			Thymio2.h
			PlaygroundViewer.h
			PlaygroundDBusAdaptors.h
		)
		
		add_executable(asebaplayground WIN32 ${playground_SRCS} ${playground_MOCS})
		
		target_link_libraries(asebaplayground asebasim asebavmbuffer asebavm ${enki_VIEWER_LIBRARIES} ${enki_LIBRARY} ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ASEBA_ZEROCONFQT_LIBRARIES} ${ASEBA_CORE_LIBRARIES} ${EXTRA_LIBS})
		install(TARGETS asebaplayground RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

	endif (QT4_FOUND)
endif (ENKI_FOUND)
