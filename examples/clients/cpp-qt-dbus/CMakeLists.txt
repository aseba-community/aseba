if(Qt5DBus_FOUND AND Qt5Test_FOUND)
	set(CMAKE_AUTOMOC ON)

	set(aseba-qt-dbus_SRCS
		main.cpp
		dbusinterface.cpp
	)

	add_executable(aseba-qt-dbus ${aseba-qt-dbus_SRCS})
	target_link_libraries(aseba-qt-dbus Qt5::DBus Qt5::Test)
endif()
