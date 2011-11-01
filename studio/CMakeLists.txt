find_package(Qt4)

set(DOC_NAME aseba-doc)

# file(TO_NATIVE_PATH ...) is broken with MSYS generator 
# (http://www.kwwidgets.org/Bug/bug_view_advanced_page.php?bug_id=5939)
# This is a replacement...
# From : http://www.cmake.org/pipermail/cmake/2006-July/010180.html
MACRO(MAKE_WINDOWS_PATH pathname)
  # An extra \\ escape is necessary to get a \ through CMake's processing.
  STRING(REPLACE "/" "\\" ${pathname} "${${pathname}}")
  # Enclose with UNESCAPED quotes.  This means we need to escape our
  # quotes once here, i.e. with \"
  SET(${pathname} \"${${pathname}}\")
ENDMACRO(MAKE_WINDOWS_PATH)

if (QT4_FOUND)
	set(QT_USE_QTXML ON)
	set(QT_USE_QTNETWORK ON)
	set(QT_USE_QTMAIN ON)
	set(QT_USE_QTHELP ON)
	include(${QT_USE_FILE})
	
	find_path(QWT_INCLUDE_DIR qwt_plot.h /usr/include/qwt-qt4 /usr/local/include/qwt-qt4)
	find_library(QWT_LIBRARIES qwt-qt4 /usr/lib /usr/local/lib)
	
	if (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
		message(STATUS "Qwt found")
		include_directories (${QWT_INCLUDE_DIR})
		set(EXTRA_LIBS ${EXTRA_LIBS} ${QWT_LIBRARIES})
		add_definitions(-DHAVE_QWT)
	else (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
		message(STATUS "Qwt not found")
	endif (QWT_LIBRARIES AND QWT_INCLUDE_DIR)

	# generates the documentation
	if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
		message("-- qcollectiongenerator executable found")
		# all the process should be done in ${CMAKE_CURRENT_SOURCE_DIR}, then the binary files are moved to ${CMAKE_CURRENT_BINARY_DIR}
		set(SOURCE_QCH "${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qch")
		set(SOURCE_QHC "${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhc")
		set(DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}")
		if (WIN32)
			MAKE_WINDOWS_PATH(SOURCE_QCH)
			MAKE_WINDOWS_PATH(SOURCE_QHC)
			MAKE_WINDOWS_PATH(DEST_DIR)
			add_custom_command(
				OUTPUT ${DOC_NAME}.qhc ${DOC_NAME}.qch
				COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhcp -o ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhc
				COMMAND move "${SOURCE_QCH}" "${DEST_DIR}"
				COMMAND move "${SOURCE_QHC}" "${DEST_DIR}"
				MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhcp ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhp)
			add_custom_target(documentation DEPENDS ${DOC_NAME}.qhc ${DOC_NAME}.qch)
		else (WIN32)
			add_custom_command(
				OUTPUT ${DOC_NAME}.qhc ${DOC_NAME}.qch
				COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhcp -o ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhc
				COMMAND mv "${SOURCE_QCH}" "${DEST_DIR}"
				COMMAND mv "${SOURCE_QHC}" "${DEST_DIR}"
				MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhcp ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhp)
			add_custom_target(documentation DEPENDS ${DOC_NAME}.qhc ${DOC_NAME}.qch)
		endif (WIN32)
	endif (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
	
	set(studio_SRCS
		AeslEditor.cpp
		MainWindow.cpp
		TargetModels.cpp
		NamedValuesVectorModel.cpp
		ClickableLabel.cpp
		DashelTarget.cpp
		CustomDelegate.cpp
		CustomWidgets.cpp
		main.cpp
		FindDialog.cpp
		Target.cpp
		EventViewer.cpp
		Plugin.cpp
		HelpViewer.cpp
		plugins/VariablesViewPlugin.cpp
		plugins/ThymioBootloader.cpp)
	
	qt4_automoc(${studio_SRCS})
	
	qt4_add_resources(resfiles asebastudio.qrc)
	
	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
	
	add_executable(asebastudio WIN32 ${studio_SRCS} ${resfiles} ${CMAKE_BINARY_DIR}/version.h)
	if (HAS_DYN_VERSION)
		add_dependencies(asebastudio versionheader)
	endif (HAS_DYN_VERSION)
	if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
		add_dependencies(asebastudio documentation)
	endif (QT_QCOLLECTIONGENERATOR_EXECUTABLE)

	target_link_libraries(asebastudio asebacompiler asebacore ${QT_LIBRARIES} ${EXTRA_LIBS})

	install(TARGETS asebastudio RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

endif (QT4_FOUND)
