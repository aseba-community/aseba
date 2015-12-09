if (QT4_FOUND AND ENKI_FOUND)
	# SDL
# 	find_package(SDL)
# 	if (SDL_FOUND)
# 		include_directories(${SDL_INCLUDE_DIR})
# 		set(EXTRA_LIBS ${SDL_LIBRARY})
# 		add_definitions("-DUSE_SDL")
# 	elseif (SDL_FOUND)
# 		set(EXTRA_LIBS "")
# 	endif (SDL_FOUND)

	find_package(OpenGL REQUIRED)
	
	set(QT_USE_QTMAIN ON)
	set(QT_USE_QTOPENGL ON)
	set(QT_USE_QTXML ON)
	include(${QT_USE_FILE})
	
	include_directories(${ENKI_INCLUDE_DIR})
	
	set(playground_SRCS
		AsebaGlue.cpp
		Door.cpp
		EPuck.cpp
		EPuck-descriptions.c
		Thymio2.cpp
		Thymio2-descriptions.c
		PlaygroundViewer.cpp
		playground.cpp
	)
	qt4_wrap_cpp(playground_MOCS
		PlaygroundViewer.h
	)
	
	add_executable(asebaplayground WIN32 ${playground_SRCS} ${playground_MOCS} ${playground_RCC_SRCS})
	
	target_link_libraries(asebaplayground asebavmbuffer asebavm ${ENKI_VIEWER_LIBRARY} ${ENKI_LIBRARY} ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ASEBA_CORE_LIBRARIES} ${EXTRA_LIBS})
	install(TARGETS asebaplayground RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif (QT4_FOUND AND ENKI_FOUND)
