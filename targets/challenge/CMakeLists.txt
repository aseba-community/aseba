if (Qt5Widgets_FOUND AND Qt5OpenGL_FOUND AND ENKI_FOUND)
	find_package(OpenGL REQUIRED)
	
	set(CMAKE_AUTOMOC ON)
	
	include_directories(${enki_INCLUDE_DIR})
	
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
	
	qt5_add_resources(challenge_RCC_SRCS challenge-textures.qrc)
	
	add_executable(asebachallenge WIN32 ${challenge_SRCS} ${challenge_RCC_SRCS})
	
	target_link_libraries(asebachallenge asebavmbuffer asebavm ${enki_VIEWER_LIBRARIES} ${enki_LIBRARY} Qt5::OpenGL Qt5::Widgets ${OPENGL_LIBRARIES} ${ASEBA_CORE_LIBRARIES})
	install(TARGETS asebachallenge RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif ()
