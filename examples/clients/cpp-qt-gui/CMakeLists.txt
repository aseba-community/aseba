if(Qt5Widgets_FOUND AND Qt5Network_FOUND)
	set(CMAKE_AUTOMOC ON)
	
	set(aseba-qt-gui_SRCS
		main.cpp
		dashelinterface.cpp
		mainwindow.cpp
	)
	
	add_executable(aseba-qt-gui WIN32 ${aseba-qt-gui_SRCS})
	target_link_libraries(aseba-qt-gui asebacommon Qt5::Widgets Qt5::Network)
endif ()
