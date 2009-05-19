find_package(Qt4)

if (QT4_FOUND)
	include(${QT_USE_FILE})
	
	find_path(QWT_INCLUDE_DIR qwt_plot.h /usr/include/qwt-qt4 /usr/local/include/qwt-qt4)
	find_library(QWT_LIBRARIES qwt-qt4 /usr/lib /usr/local/lib)
	
	set(studio_SRCS
		AeslEditor.cpp
		MainWindow.cpp
		TargetModels.cpp
		NamedValuesVectorModel.cpp
		ClickableLabel.cpp
		DashelTarget.cpp
		CustomDelegate.cpp
		main.cpp
		Target.cpp
		VariablesViewPlugin.cpp
		EventViewer.cpp)
	
	qt4_automoc(${studio_SRCS})
	
	qt4_add_resources(resfiles asebastudio.qrc)
	
	include_directories (${QT_INCLUDE_DIR} ${QT_QTXML_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/studio)
	
	add_executable(asebastudio WIN32 ${studio_SRCS} ${resfiles})
	
	if (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
		message(STATUS "Qwt found")
		include_directories (${QWT_INCLUDE_DIR})
		set(EXTRA_LIBS ${EXTRA_LIBS} ${QWT_LIBRARIES})
		add_definitions(-DHAVE_QWT)
	else (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
		message(STATUS "Qwt not found")
	endif (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
	
	# Only include the qtmain lib if we are on the windows platform
	if (WIN32)
		target_link_libraries(asebastudio asebacompiler asebacore ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} ${QT_QTXML_LIBRARIES} ${QT_QTMAIN_LIBRARY} ${EXTRA_LIBS})
	else (WIN32)
		target_link_libraries(asebastudio asebacompiler asebacore ${QT_LIBRARIES} ${QT_QTNETWORK_LIBRARIES} ${QT_QTXML_LIBRARIES} ${EXTRA_LIBS})
	endif (WIN32)
endif (QT4_FOUND)