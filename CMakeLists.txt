# emacs: -*- CMake -*-
# kate: replace-tabs off; tab-width 4; indent-width 4; tab-indents true; indent-mode normal
# vim: ts=4:sw=4:noexpandtab

# project name
project(aseba)

# ensure a minimal version of CMake
cmake_minimum_required(VERSION 3.5)

# additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)
include(CMakeModules/ParseVersion.cmake)
include(CMakeModules/cpp_features.cmake)
include(CMakeModules/workarounds.cmake)

include(CMakeModules/aseba_conf.cmake)

# testing and defines
enable_testing()
add_definitions(-DASEBA_ASSERT)

# Dashel
find_package(dashel REQUIRED)
include_directories(${dashel_INCLUDE_DIRS})

# Enki
find_package(enki)

# Qt and Qwt
find_package(Qt5 5.6 COMPONENTS Core Gui Widgets Network Help Concurrent Xml Svg X11Extras OPTIONAL_COMPONENTS Test Multimedia)

find_package(Qwt)

if (UNIX AND NOT APPLE)
	find_package(Qt5DBus 5.6 REQUIRED)
endif ()

# Zeroconf
find_package(DNSSD)
if (DNSSD_FOUND)
	if(AVAHI_COMMON_LIBRARIES)
		add_definitions(-DDNSSD_AVAHI)
	endif(AVAHI_COMMON_LIBRARIES)
	include_directories(${DNSSD_INCLUDE_DIRS})
	find_package(Threads REQUIRED)
	add_definitions(-DZEROCONF_SUPPORT)
endif()

# Aseba itself
set(LIB_INSTALL_DIR lib CACHE FILEPATH "Where to install libraries")
set(LIB_VERSION_MAJOR 3) # Must be bumped for incompatible ABI changes
set(LIB_VERSION_MINOR 0)
set(LIB_VERSION_PATCH 0)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})

# Core libraries
add_subdirectory(common)
add_subdirectory(transport)

# define the core libraries variable, to be used by all Aseba programs
set(ASEBA_CORE_LIBRARIES asebadashelplugins asebacommon ${dashel_LIBRARIES})
if (DNSSD_FOUND)
	if (AVAHI_COMMON_LIBRARIES)
		set(ASEBA_ZEROCONF_ADDITIONAL_LIBRARIES ${AVAHI_CLIENT_LIBRARIES} ${AVAHI_COMMON_LIBRARIES} ${DNSSD_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
	else()
		set(ASEBA_ZEROCONF_ADDITIONAL_LIBRARIES ${DNSSD_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
		if (WIN32)
			set(ASEBA_ZEROCONF_ADDITIONAL_LIBRARIES ${ASEBA_ZEROCONF_ADDITIONAL_LIBRARIES} -lws2_32)
		endif()
	endif()
	set(ASEBA_ZEROCONF_LIBRARIES asebazeroconf ${ASEBA_ZEROCONF_ADDITIONAL_LIBRARIES})
	if (Qt5Core_FOUND)
		set(ASEBA_ZEROCONFQT_LIBRARIES asebazeroconfqt asebazeroconf ${ASEBA_ZEROCONF_ADDITIONAL_LIBRARIES})
	endif()
endif()

# Target and client specific libraries
add_subdirectory(vm)
add_subdirectory(compiler)

# User programs
add_subdirectory(clients)
add_subdirectory(switches)
add_subdirectory(targets)

# Examples for developers
add_subdirectory(examples)

# Unit tests
add_subdirectory(tests)

# User entries
if (UNIX)
	add_subdirectory(menu)
endif()
