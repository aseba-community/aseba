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
	
	set(vrcs_SRCS vrcs.cpp objects/food_base.cpp  objects/food_charge0.cpp  objects/food_charge1.cpp  objects/food_charge2.cpp  objects/food_charge3.cpp  objects/food_ring.cpp)
	
	qt4_automoc(${vrcs_SRCS})
	
	qt4_add_resources(vrcs_RCC_SRCS vrcs-textures.qrc)
	
	include_directories (${QT_INCLUDE_DIR} ${QT_QTNETWORK_INCLUDE_DIR} ${QT_QTOPENGL_INCLUDE_DIR} .)
	
	add_executable(vrcs ${vrcs_SRCS} ${vrcs_RCC_SRCS})
	#add_executable(vrcs ${vrcs_SRCS} )
	
	target_link_libraries(vrcs aseba ${ENKI_VIEWER_LIBRARY} ${ENKI_LIBRARY} ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} ${QT_QTOPENGL_LIBRARIES}  ${EXTRA_LIBS} ${OPENGL_LIBRARIES})

endif (QT4_FOUND)
