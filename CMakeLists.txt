# kate: replace-tabs off; tab-width 4; indent-width 4; tab-indents true; indent-mode normal
# vim: ts=4:sw=4:noexpandtab
cmake_minimum_required(VERSION 2.6)

project(aseba)
enable_testing()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
include(CMakeModules/ParseVersion.cmake)
include(ExternalProject)

add_definitions(-Wall)
add_definitions(-DASEBA_ASSERT)

# Remove -Wl,--no-undefined which CMake 3.0.2 (on OpenSUSE 13.2) adds to the
# linker options when building shared libs. That breaks building libs that use
# callbacks that will be provided by other libs when the executable is linked.
if (DEFINED CMAKE_SHARED_LINKER_FLAGS)
	STRING(REPLACE "-Wl,--no-undefined" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
endif(DEFINED CMAKE_SHARED_LINKER_FLAGS)

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
set(ENKI_INCLUDE_DIR "${aseba_SOURCE_DIR}/thirdparty/enki")
set(ENKI_LIBRARY "${aseba_BINARY_DIR}/thirdparty/enki/libenki.a")
set(ENKI_VIEWER_LIBRARY "${aseba_BINARY_DIR}/thirdparty/enki/libenkiviewer.a")
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
