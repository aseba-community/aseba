if (QT4_FOUND AND ENKI_FOUND)
	find_package(OpenGL REQUIRED)
	
	set(QT_USE_QTMAIN ON)
	set(QT_USE_QTOPENGL ON)
	include(${QT_USE_FILE})
	
	include_directories(${ENKI_INCLUDE_DIR})
	
	set(challenge_SRCS
		challenge.cpp
		challenge-vm-description.c
		objects/food_base.cpp
		objects/food_charge0.cpp
		objects/food_charge1.cpp
		objects/food_charge2.cpp
		objects/food_charge3.cpp
		objects/food_ring.cpp
	)
	qt4_wrap_cpp(challenge_MOCS
		challenge.h
	)
	
	qt4_add_resources(challenge_RCC_SRCS challenge-textures.qrc)
	
	add_executable(asebachallenge WIN32 ${challenge_SRCS} ${challenge_RCC_SRCS} ${challenge_MOCS})
	
	target_link_libraries(asebachallenge asebavmbuffer asebavm ${ENKI_VIEWER_LIBRARY} ${ENKI_LIBRARY} ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ASEBA_CORE_LIBRARIES})
	install(TARGETS asebachallenge RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif (QT4_FOUND AND ENKI_FOUND)
