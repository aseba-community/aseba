if (QT4_FOUND AND ENKI_FOUND)
	find_package(OpenGL REQUIRED)
	
	set(QT_USE_QTMAIN ON)
	set(QT_USE_QTOPENGL ON)
	include(${QT_USE_FILE})
	
	include_directories(${ENKI_INCLUDE_DIR})
	
	set(enki_SRCS
		AsebaMarxbot.cpp
		EnkiMarxbot.cpp
		AsebaMarxbot-vm-descriptions.c
	)
	
	add_executable(asebamarxbot WIN32 ${enki_SRCS})
	
	target_link_libraries(asebamarxbot asebavmbuffer asebavm ${ENKI_VIEWER_LIBRARY} ${ENKI_LIBRARY} ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

endif (QT4_FOUND AND ENKI_FOUND)
