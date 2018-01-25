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
include(CMakeModules/dashel.cmake)
include(CMakeModules/enki.cmake)

# testing and defines
enable_testing()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")


# Qt and Qwt
find_package(Qt5 5.6 COMPONENTS Core Gui Widgets Network Help Concurrent Xml Svg X11Extras OPTIONAL_COMPONENTS Test Multimedia)

find_package(Qwt)

if (UNIX AND NOT APPLE)
	find_package(Qt5DBus 5.6 REQUIRED)
endif ()

# Zeroconf
include(CMakeModules/zeroconf.cmake)


# Aseba itself
set(LIB_INSTALL_DIR lib CACHE FILEPATH "Where to install libraries")
set(LIB_VERSION_MAJOR 3) # Must be bumped for incompatible ABI changes
set(LIB_VERSION_MINOR 0)
set(LIB_VERSION_PATCH 0)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})


# define the core libraries variable, to be used by all Aseba programs
set(ASEBA_CORE_LIBRARIES asebadashelplugins asebacommon dashel)

add_subdirectory(aseba)

# Examples for developers
add_subdirectory(examples)

# Unit tests
add_subdirectory(tests)

# User entries
if (UNIX)
	add_subdirectory(menu)
endif()
