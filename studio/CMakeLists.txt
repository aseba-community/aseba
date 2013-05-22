# kate: replace-tabs off; tab-width 4; indent-width 4; tab-indents true; indent-mode normal
# vim: ts=4:sw=4:noexpandtab
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
	set(QT_USE_QTCONCURRENT ON)
	set(QT_USE_QTSVG ON)
	include(${QT_USE_FILE})

	# add a console for debug messages under Windows
	if (WIN32)
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-Wl,-subsystem,console ${CMAKE_EXE_LINKER_FLAGS}")
	endif (WIN32)
	
	find_path(QWT_INCLUDE_DIR qwt_plot.h /usr/include/qwt-qt4 /usr/local/include/qwt-qt4 /usr/include/qwt /usr/local/include/qwt)
	find_library(QWT_LIBRARIES qwt-qt4)
	
	if (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
		message(STATUS "Qwt found")
		include_directories (${QWT_INCLUDE_DIR})
		set(EXTRA_LIBS ${EXTRA_LIBS} ${QWT_LIBRARIES})
		add_definitions(-DHAVE_QWT)
	else (QWT_LIBRARIES AND QWT_INCLUDE_DIR)
		message(STATUS "Qwt not found")
	endif (QWT_LIBRARIES AND QWT_INCLUDE_DIR)

	# generates the documentation
	if (QT_QTHELP_FOUND AND QT_QCOLLECTIONGENERATOR_EXECUTABLE)
		message("-- qcollectiongenerator executable found")
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhc ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qch
			COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ARGS ${DOC_NAME}.qhcp -o ${DOC_NAME}.qhc
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhcp ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhp)
	else (QT_QTHELP_FOUND AND QT_QCOLLECTIONGENERATOR_EXECUTABLE)
		message(WARNING "-- Qt help support missing, studio online help disabled (hint: on Ubuntu, install package qt4-dev-tools)")
	endif (QT_QTHELP_FOUND AND QT_QCOLLECTIONGENERATOR_EXECUTABLE)
	
	# code common to the use of Aseba under Qt
	set(qt_common_SRCS
		Target.cpp
		DashelTarget.cpp
		TargetModels.cpp
		CustomWidgets.cpp
		AeslEditor.cpp
		translations/CompilerTranslator.cpp
	)
	qt4_wrap_cpp(qt_common_MOCS
		Target.h
		DashelTarget.h
		TargetModels.h
		AeslEditor.h
		translations/CompilerTranslator.h
	)
	add_library(asebaqtcommon ${qt_common_SRCS} ${qt_common_MOCS})
	
	# plugins
	set(qt_plugins_SRCS
		Plugin.cpp
		plugins/VariablesViewPlugin.cpp
		plugins/ThymioVPL/ThymioVisualProgramming.cpp
		plugins/ThymioVPL/ThymioScene.cpp
		plugins/ThymioVPL/ThymioButtons.cpp
		plugins/ThymioVPL/ThymioEventButtons.cpp
		plugins/ThymioVPL/ThymioActionButtons.cpp
		plugins/ThymioVPL/ThymioIRButtons.cpp
		plugins/ThymioVPL/ThymioIRVisitors.cpp
		plugins/ThymioVPL/ThymioCompiler.cpp
	)
	qt4_wrap_cpp(qt_plugins_MOCS
		plugins/VariablesViewPlugin.h
		plugins/ThymioVPL/ThymioVisualProgramming.h
		plugins/ThymioVPL/ThymioScene.h
		plugins/ThymioVPL/ThymioButtons.h
	)
	add_library(asebaqtplugins ${qt_plugins_SRCS} ${qt_plugins_MOCS})
	
	qt4_add_resources(resfiles asebastudio.qrc)
	
	include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

	if (QT_QTHELP_FOUND)
		# studio for desktop use
		set(studio_SRCS
			StudioAeslEditor.cpp
			PluginRegistry.cpp
			MainWindow.cpp
			NamedValuesVectorModel.cpp
			ClickableLabel.cpp
			CustomDelegate.cpp
			main.cpp
			FindDialog.cpp
			EventViewer.cpp
			HelpViewer.cpp
			ConfigDialog.cpp
			ModelAggregator.cpp
		)
		qt4_wrap_cpp(studio_MOCS
			StudioAeslEditor.h
			MainWindow.h
			NamedValuesVectorModel.h
			ClickableLabel.h
			CustomDelegate.h
			FindDialog.h
			EventViewer.h
			HelpViewer.h
			ConfigDialog.h
			ModelAggregator.h
		)

		if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
			qt4_add_resources(resfiles asebastudiohelp.qrc)
		endif (QT_QCOLLECTIONGENERATOR_EXECUTABLE)

		add_executable(asebastudio WIN32 ${studio_SRCS} ${studio_MOCS} ${resfiles} ${CMAKE_BINARY_DIR}/version.h)
		if (HAS_DYN_VERSION)
			add_dependencies(asebastudio versionheader)
		endif (HAS_DYN_VERSION)

		target_link_libraries(asebastudio asebaqtplugins asebaqtcommon asebacompiler asebacore ${QT_LIBRARIES} ${EXTRA_LIBS})

		install(TARGETS asebastudio RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
	endif (QT_QTHELP_FOUND)
	
	# Thymio VPL standalone
	set(thymiovpl_SRCS
		ThymioVPLStandalone.cpp
		thymiovpl.cpp
	)
	qt4_wrap_cpp(thymiovpl_MOCS
		ThymioVPLStandalone.h
	)

	if (ANDROID)
		add_library(thymiovpl SHARED ${thymiovpl_SRCS} ${thymiovpl_MOCS} ${resfiles} ${CMAKE_BINARY_DIR}/version.h)
	else (ANDROID)
		add_executable(thymiovpl WIN32 ${thymiovpl_SRCS} ${thymiovpl_MOCS} ${resfiles} ${CMAKE_BINARY_DIR}/version.h)
	endif (ANDROID)
	if (HAS_DYN_VERSION)
		add_dependencies(thymiovpl versionheader)
	endif (HAS_DYN_VERSION)

	target_link_libraries(thymiovpl asebaqtplugins asebaqtcommon asebacompiler asebacore ${QT_LIBRARIES} ${EXTRA_LIBS})

	install(TARGETS thymiovpl RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
endif (QT4_FOUND)
