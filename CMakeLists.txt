# kate: replace-tabs off; tab-width 4; indent-width 4; tab-indents true; indent-mode normal
# vim: ts=4:sw=4:noexpandtab

# project name
project(aseba)

# ensure a minimal version of CMake
cmake_minimum_required(VERSION 2.8.10)

# additional CMake modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
include(CMakeModules/ParseVersion.cmake)
include(ExternalProject)

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
endif(DEFINED CMAKE_SHARED_LINKER_FLAGS)

# testing and defines
enable_testing()
add_definitions(-Wall)
add_definitions(-DASEBA_ASSERT)

# Dashel
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared instead of static libs")
add_subdirectory("${aseba_SOURCE_DIR}/thirdparty/dashel")
include_directories("${aseba_SOURCE_DIR}/thirdparty/dashel")
if (APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    set(DASHEL_LIBRARIES
        "${aseba_BINARY_DIR}/thirdparty/dashel/libdashel.a"
        ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY}
        )
elseif(WIN32)
    set(DASHEL_LIBRARIES
        "${aseba_BINARY_DIR}/thirdparty/dashel/libdashel.a"
		winspool ws2_32 setupapi
        )
else()
    set(DASHEL_LIBRARIES
        "${aseba_BINARY_DIR}/thirdparty/dashel/libdashel.a"
        )
endif()


# Enki
set(ENKI_FOUND 1)
set(ENKI_INCLUDE_DIR "${aseba_SOURCE_DIR}/thirdparty/enki")
set(ENKI_LIBRARY "${aseba_BINARY_DIR}/thirdparty/enki/enki/libenki.a")
set(ENKI_VIEWER_LIBRARY "${aseba_BINARY_DIR}/thirdparty/enki/viewer/libenkiviewer.a")
include_directories("${ENKI_INCLUDE_DIR}" "${aseba_BINARY_DIR}/thirdparty/enki/viewer")
add_subdirectory("${aseba_SOURCE_DIR}/thirdparty/enki" "${aseba_BINARY_DIR}/thirdparty/enki")

# Qt
if (UNIX AND NOT APPLE)
	find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork QtXml QtHelp QtSvg QtWebKit QtDBus)
else (UNIX AND NOT APPLE)
	find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork QtXml QtHelp QtSvg QtWebKit)
endif (UNIX AND NOT APPLE)

# Qwt
ExternalProject_Add(qwt
    PREFIX ${aseba_BINARY_DIR}/thirdparty/qwt
    SOURCE_DIR ${aseba_SOURCE_DIR}/thirdparty/qwt
    CONFIGURE_COMMAND qmake PREFIX=${aseba_BINARY_DIR}/thirdparty/qwt ${aseba_SOURCE_DIR}/thirdparty/qwt
    BUILD_COMMAND make
)
if(APPLE)
    set(QWT_INCLUDE_DIR "${aseba_BINARY_DIR}/thirdparty/qwt/src/qwt-build/lib/qwt.framework/Headers")


else()
endif()

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
set(ASEBA_CORE_LIBRARIES asebadashelplugins asebacommon ${DASHEL_LIBRARIES})

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
