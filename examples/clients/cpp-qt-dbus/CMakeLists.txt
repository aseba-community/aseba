include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

find_package(Qt4 REQUIRED QtDBus)

if(COMPILER_SUPPORTS_CXX0X)
	if(QT4_FOUND)
		set(CMAKE_CXX_FLAGS "-std=c++0x")

		set(QT_USE_QTNETWORK ON)
		set(QT_USE_QTMAIN ON)
		include(${QT_USE_FILE})
	
		set(qt-dbus_SRCS
			main.cpp
			dbusinterface.cpp
		)

		set(qt-dbus_MOC_HDRS
			dbusinterface.h
		)

		#qt4_automoc(${qt-dbus_SRCS})
		qt4_wrap_cpp(qt-dbus_MOC_SRCS ${qt-dbus_MOC_HDRS})
	
		include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

		add_executable(qt-dbus ${qt-dbus_SRCS} ${qt-dbus_MOC_SRCS})
		target_link_libraries(qt-dbus ${QT_LIBRARIES})
		install(TARGETS qt-dbus RUNTIME DESTINATION bin)
	endif()
else()
	message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support, so we will skip building the example interface qt-dbus.")
endif()

