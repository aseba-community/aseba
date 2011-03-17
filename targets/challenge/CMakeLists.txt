find_package(Qt4)

if (QT4_FOUND AND ENKI_FOUND)
	find_package(OpenGL REQUIRED)
	
	include(${QT_USE_FILE})
	
	include_directories(${ENKI_INCLUDE_DIR})
	
	set(challenge_SRCS challenge.cpp challenge-vm-description.c objects/food_base.cpp  objects/food_charge0.cpp  objects/food_charge1.cpp  objects/food_charge2.cpp  objects/food_charge3.cpp  objects/food_ring.cpp)
	
	qt4_automoc(${challenge_SRCS})
	
	qt4_add_resources(challenge_RCC_SRCS challenge-textures.qrc)
	
	include_directories (${QT_INCLUDE_DIR} ${QT_QTNETWORK_INCLUDE_DIR} ${QT_QTOPENGL_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/targets/challenge .)
	
	add_executable(asebachallenge WIN32 ${challenge_SRCS} ${challenge_RCC_SRCS})
	
	
	if (WIN32)
		target_link_libraries(asebachallenge asebavmbuffer asebavm asebacore ${ENKI_VIEWER_LIBRARY} ${ENKI_LIBRARY} ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} ${QT_QTOPENGL_LIBRARIES} ${QT_QTMAIN_LIBRARY} ${EXTRA_LIBS} ${OPENGL_LIBRARIES})
	else (WIN32)
		target_link_libraries(asebachallenge asebavmbuffer asebavm asebacore  ${ENKI_VIEWER_LIBRARY} ${ENKI_LIBRARY} ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} ${QT_QTOPENGL_LIBRARIES} ${EXTRA_LIBS} ${OPENGL_LIBRARIES})
		install(TARGETS asebachallenge RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
	endif (WIN32)

endif (QT4_FOUND AND ENKI_FOUND)
