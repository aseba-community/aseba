include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

if(COMPILER_SUPPORTS_CXX0X)
	if(QT4_FOUND AND QT_QTDBUS_FOUND)
		set(CMAKE_CXX_FLAGS "-std=c++0x")

		set(QT_USE_QTMAIN ON)
		set(QT_USE_QTDBUS ON)
		set(QT_USE_QTTEST ON)
		include(${QT_USE_FILE})
	
		set(aseba-qt-dbus_SRCS
			main.cpp
			dbusinterface.cpp
		)

		set(aseba-qt-dbus_MOC_HDRS
			dbusinterface.h
		)

		qt4_wrap_cpp(aseba-qt-dbus_MOC_SRCS ${aseba-qt-dbus_MOC_HDRS})
	
		include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

		add_executable(aseba-qt-dbus ${aseba-qt-dbus_SRCS} ${aseba-qt-dbus_MOC_SRCS})
		target_link_libraries(aseba-qt-dbus ${QT_LIBRARIES})
	endif()
else()
	message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support, so we will skip building the example interface qt-dbus.")
endif()

