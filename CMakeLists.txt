# kate: replace-tabs off; tab-width 4; indent-width 4; tab-indents true; indent-mode normal
# vim: ts=4:sw=4:noexpandtab

# project name
project(aseba)

# ensure a minimal version of CMake
cmake_minimum_required(VERSION 2.8.10)

# additional CMake modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
include(CMakeModules/ParseVersion.cmake)

# check the compiler version as we need full C++11 support
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# using Clang
	if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.4")
		message(FATAL_ERROR, "Your clang compiler has version ${CMAKE_CXX_COMPILER_VERSION}, while version 3.4 or later is required")
	endif ()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	# using AppleClang
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.1")
		message(FATAL_ERROR "Your XCode environment has version ${CMAKE_CXX_COMPILER_VERSION}, while version 5.1 or later is required")
	endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# using GCC
	if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
		message(FATAL_ERROR, "Your g++ compiler has version ${CMAKE_CXX_COMPILER_VERSION}, while version 4.9 or later is required")
	endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# using MSVC
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.0.23506")
		message(FATAL_ERROR "Your Microsoft Visual C++ compiler has version ${CMAKE_CXX_COMPILER_VERSION}, while version MSVC 2015 Update 1+ is required")
	endif()
endif ()

# enable C++11 support
if (CMAKE_VERSION VERSION_LESS "3.1")
	if (MSVC)
		message(FATAL_ERROR, "CMake version 3.1 or later is required to compiler Aseba with Microsoft Visual C++")
	endif ()
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set (CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
	else ()
		set (CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
	endif ()
else ()
	set (CMAKE_CXX_STANDARD 11)
endif ()

# Remove -Wl,--no-undefined which CMake 3.0.2 (on OpenSUSE 13.2) adds to the
# linker options when building shared libs. That breaks building libs that use
# callbacks that will be provided by other libs when the executable is linked.
if (DEFINED CMAKE_SHARED_LINKER_FLAGS)
	STRING(REPLACE "-Wl,--no-undefined" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
endif (DEFINED CMAKE_SHARED_LINKER_FLAGS)

# testing and defines
enable_testing()
add_definitions(-Wall)
add_definitions(-DASEBA_ASSERT)

# Dashel
find_package(dashel REQUIRED)
include_directories(${dashel_INCLUDE_DIRS})

# Enki
find_package(enki)

# Qt
find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork QtXml QtHelp QtSvg QtWebKit OPTIONAL_COMPONENTS QtDBus)

# Qwt
find_path(QWT_INCLUDE_DIR qwt_plot.h /usr/include/qwt5-qt4 /usr/include/qwt-qt4 /usr/local/include/qwt-qt4 /usr/include/qwt /usr/local/include/qwt)
find_library(QWT_LIBRARIES NAMES qwt5-qt4 qwt-qt4 qwt PATHS /usr/lib /usr/local/lib)

# Aseba itself
set(LIB_INSTALL_DIR lib CACHE FILEPATH "Where to install libraries")
set(LIB_VERSION_MAJOR 2) # Must be bumped for incompatible ABI changes
set(LIB_VERSION_MINOR 0)
set(LIB_VERSION_PATCH 0)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})

# Core libraries
add_subdirectory(common)
add_subdirectory(transport)

# define the core libraries variable, to be used by all Aseba programs
set(ASEBA_CORE_LIBRARIES asebadashelplugins asebacommon ${dashel_LIBRARIES})

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
