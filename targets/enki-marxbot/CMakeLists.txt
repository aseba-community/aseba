find_package(Qt4)
find_package(OpenGL REQUIRED)

if (QT4_FOUND)

	find_path(ENKI_INCLUDE_DIR enki/PhysicalEngine.h
		/usr/include/enki
		/usr/local/include/enki
	)

	find_library(ENKI_LIBRARY enki
		/usr/lib
		/usr/local/lib
	)
	
	find_library(ENKI_VIEWER_LIBRARY enkiviewer
		/usr/lib
		/usr/local/lib
	)
	
	include(${QT_USE_FILE})
	
	include_directories(${ENKI_INCLUDE_DIR})
	
	
	set(enki_SRCS
		AsebaMarxbot.cpp
		EnkiMarxbot.cpp
	)
	
	qt4_automoc(${enki_SRCS})
	
	include_directories (${QT_INCLUDE_DIR} ${QT_QTNETWORK_INCLUDE_DIR} ${QT_QTOPENGL_INCLUDE_DIR} .)
	
	add_executable(asebamarxbot WIN32 ${enki_SRCS})
	
	
	if (WIN32)
		target_link_libraries(asebamarxbot aseba ${ENKI_VIEWER_LIBRARY} ${ENKI_LIBRARY} ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} ${QT_QTOPENGL_LIBRARIES} ${QT_QTMAIN_LIBRARY} ${EXTRA_LIBS} ${OPENGL_LIBRARIES})
	else (WIN32)
		target_link_libraries(asebamarxbot aseba ${ENKI_VIEWER_LIBRARY} ${ENKI_LIBRARY} ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} ${QT_QTOPENGL_LIBRARIES}  ${EXTRA_LIBS} ${OPENGL_LIBRARIES})
	endif (WIN32)

endif (QT4_FOUND)
