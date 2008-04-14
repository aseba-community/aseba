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
	
	set(challenge_SRCS challenge.cpp objects/food_base.cpp  objects/food_charge0.cpp  objects/food_charge1.cpp  objects/food_charge2.cpp  objects/food_charge3.cpp  objects/food_ring.cpp)
	
	qt4_automoc(${challenge_SRCS})
	
	qt4_add_resources(challenge_RCC_SRCS challenge-textures.qrc)
	
	include_directories (${QT_INCLUDE_DIR} ${QT_QTNETWORK_INCLUDE_DIR} ${QT_QTOPENGL_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/targets/challenge .)
	
	add_executable(asebachallenge ${challenge_SRCS} ${challenge_RCC_SRCS})
	
	target_link_libraries(asebachallenge aseba ${ENKI_VIEWER_LIBRARY} ${ENKI_LIBRARY} ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} ${QT_QTOPENGL_LIBRARIES}  ${EXTRA_LIBS} ${OPENGL_LIBRARIES})

endif (QT4_FOUND)
