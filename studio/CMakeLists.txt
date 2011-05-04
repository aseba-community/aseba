find_package(Qt4)

if (QT4_FOUND)
	set(QT_USE_QTXML ON)
	set(QT_USE_QTNETWORK ON)
	set(QT_USE_QTMAIN ON)
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
		FindDialog.cpp
		Target.cpp
		EventViewer.cpp
		Plugin.cpp
		plugins/VariablesViewPlugin.cpp
		plugins/ThymioBootloader.cpp)
	
	qt4_automoc(${studio_SRCS})
	
	qt4_add_resources(resfiles asebastudio.qrc)
	
	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
	
	add_executable(asebastudio WIN32 ${studio_SRCS} ${resfiles} ${CMAKE_BINARY_DIR}/version.h)
	if (HAS_DYN_VERSION)
		add_dependencies(asebastudio versionheader)
	endif (HAS_DYN_VERSION)
	
	if (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
		message(STATUS "Qwt found")
		include_directories (${QWT_INCLUDE_DIR})
		set(EXTRA_LIBS ${EXTRA_LIBS} ${QWT_LIBRARIES})
		add_definitions(-DHAVE_QWT)
	else (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
		message(STATUS "Qwt not found")
	endif (QWT_LIBRARIES AND QWT_INCLUDE_DIR)

	target_link_libraries(asebastudio asebacompiler asebacore ${QT_LIBRARIES} ${EXTRA_LIBS})

	install(TARGETS asebastudio RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif (QT4_FOUND)
