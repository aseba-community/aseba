set (ASEBASIM_SRC
	EnkiGlue.cpp
	AsebaGlue.cpp
	DirectAsebaGlue.cpp
	Door.cpp
	robots/e-puck/EPuck.cpp
	robots/e-puck/EPuck-descriptions.c
	robots/thymio2/Thymio2.cpp
	robots/thymio2/Thymio2-natives.cpp
	robots/thymio2/Thymio2-descriptions.c
)

add_library(asebasim STATIC ${ASEBASIM_SRC})
set_target_properties(asebasim PROPERTIES VERSION ${LIB_VERSION_STRING}
										SOVERSION ${LIB_VERSION_MAJOR})


target_link_libraries(asebasim PUBLIC aseba_conf enki)

install(TARGETS asebasim
		LIBRARY DESTINATION ${LIB_INSTALL_DIR}
		ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

if (Qt5Widgets_FOUND AND Qt5OpenGL_FOUND AND Qt5Xml_FOUND)
	find_package(OpenGL REQUIRED)
	if (Qt5DBus_FOUND AND NOT WIN32)
		# force disable D-Bus on Windows, because it generates compilation
		# errors in interaction with QtOpenGL includes
		set(EXTRA_LIBS ${EXTRA_LIBS} Qt5::DBus)
		add_definitions("-DHAVE_DBUS")
	endif ()
	set(CMAKE_AUTOMOC ON)

	qt5_add_resources(resfiles asebaplayground.qrc)

	set(playground_SRCS
		DashelAsebaGlue.cpp
		PlaygroundViewer.cpp
		PlaygroundDBusAdaptors.cpp
		playground.cpp
	)

	add_executable(asebaplayground WIN32 ${playground_SRCS} ${playground_MOCS} ${resfiles})

	target_link_libraries(asebaplayground asebasim asebacommon asebavmbuffer asebavm asebaqtabout enkiviewer Qt5::Xml Qt5::Svg Qt5::Network ${EXTRA_LIBS})

	install_qt_app(asebaplayground)

	if(HAS_ZEROCONF_SUPPORT)
		target_link_libraries(asebaplayground asebazeroconfqt)
	endif()

endif ()
