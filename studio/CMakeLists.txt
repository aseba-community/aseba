find_package(Qt4)

if (QT4_FOUND)
	include(${QT_USE_FILE})
	
	set(studio_SRCS
		AeslEditor.cpp
		MainWindow.cpp
		TargetModels.cpp
		NamedValuesVectorModel.cpp
		ClickableLabel.cpp
		DashelTarget.cpp
		CustomDelegate.cpp
		main.cpp
		Target.cpp)
	
	qt4_automoc(${studio_SRCS})
	
	qt4_add_resources(resfiles asebastudio.qrc)
	
	include_directories (${QT_INCLUDE_DIR} ${QT_QTXML_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/studio)
	
	add_executable(asebastudio WIN32 ${studio_SRCS} ${resfiles})
	
	# Only include the qtmain lib if we are on the windows platform
	if (WIN32)
		target_link_libraries(asebastudio aseba ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} ${QT_QTXML_LIBRARIES} ${QT_QTMAIN_LIBRARY} ${EXTRA_LIBS})
	elseif (WIN32)
		target_link_libraries(asebastudio aseba ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} ${QT_QTXML_LIBRARIES} ${EXTRA_LIBS})
	endif (WIN32)
	
	
	
# 	find_package(OpenGL REQUIRED)
# 	
# 	include(${QT_USE_FILE})
	
# 	set(viewer_lib_SRCS Viewer.cpp objects/EPuckBody.cpp objects/EPuckRest.cpp objects/EPuckRing.cpp objects/EPuckWheelLeft.cpp objects/EPuckWheelRight.cpp)
# 	set(viewer_test_SRCS Studio.cpp)
# 	
# 	qt4_automoc(${viewer_lib_SRCS})
# 	qt4_automoc(${viewer_test_SRCS})
# 	
# 	qt4_add_resources(viewer_lib_RCC_SRCS textures.qrc)
# 	
# 	include_directories (${enki_SOURCE_DIR} ${QT_INCLUDE_DIR} ${QT_QTOPENGL_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
# 	
# 	link_directories (${enki_BINARY_DIR})
# 	
# 	add_library (enkiviewer ${viewer_lib_SRCS} ${viewer_lib_RCC_SRCS})
# 	add_executable(enkistudio ${viewer_test_SRCS})
# 	
# 	target_link_libraries(enkistudio enkiviewer enki ${QT_QTOPENGL_LIBRARIES} ${QT_LIBRARIES} ${OPENGL_LIBRARIES})
	
endif (QT4_FOUND)