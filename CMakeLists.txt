# kate: replace-tabs off; tab-width 4; indent-width 4; tab-indents true; indent-mode normal
# vim: ts=4:sw=4:noexpandtab
cmake_minimum_required(VERSION 2.6)

project(aseba)
enable_testing()

include(ParseVersion.cmake)

add_definitions(-Wall)
add_definitions(-DASEBA_ASSERT)

# Dashel

find_path(DASHEL_INCLUDE_DIR dashel/dashel.h CMAKE_FIND_ROOT_PATH_BOTH)

find_library(DASHEL_LIBRARY dashel CMAKE_FIND_ROOT_PATH_BOTH)

include_directories(${DASHEL_INCLUDE_DIR})
set(EXTRA_LIBS ${DASHEL_LIBRARY})

# libudev

find_path(UDEV_INCLUDE_DIR libudev.h)
find_library(UDEV_LIBS udev)

if (UDEV_INCLUDE_DIR AND UDEV_LIBS)
	message(STATUS "libudev found")
	set(UDEV_FOUND 1)
endif (UDEV_INCLUDE_DIR AND UDEV_LIBS)

# Dashel dependencies

if (WIN32)
	set(EXTRA_LIBS ${EXTRA_LIBS} winspool ws2_32 setupapi)
else (WIN32)
	if (APPLE)
		find_library(COREFOUNDATION_LIBRARY CoreFoundation)
		find_library(IOKIT_LIBRARY IOKit)
		mark_as_advanced(COREFOUNDATION_LIBRARY)
		mark_as_advanced(IOKIT_LIBRARY)
		set(EXTRA_LIBS ${EXTRA_LIBS} ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY} ${EXTRA_LIBS})
	else (APPLE)
		if (UDEV_FOUND)
			include_directories(${UDEV_INCLUDE_DIR})
			set(EXTRA_LIBS ${EXTRA_LIBS} ${UDEV_LIBS} ${EXTRA_LIBS})
		else (UDEV_FOUND)
			find_package(PkgConfig)
			pkg_check_modules(HAL hal)
			if (${HAL_FOUND})
				include_directories(${HAL_INCLUDE_DIRS})
				set(EXTRA_LIBS ${EXTRA_LIBS} ${HAL_LIBRARIES} ${EXTRA_LIBS})
			endif (${HAL_FOUND})
		endif (UDEV_FOUND)
	endif (APPLE)
endif (WIN32)

# Enki

find_path(ENKI_INCLUDE_DIR enki/PhysicalEngine.h)
find_library(ENKI_LIBRARY enki)
find_library(ENKI_VIEWER_LIBRARY enkiviewer)
if (ENKI_INCLUDE_DIR AND ENKI_LIBRARY AND ENKI_VIEWER_LIBRARY)
	set(ENKI_FOUND 1)
	message(STATUS "Enki found")
else (ENKI_INCLUDE_DIR AND ENKI_LIBRARY AND ENKI_VIEWER_LIBRARY)
	message(STATUS "Enki *not* found, simulation targets disabled")
endif (ENKI_INCLUDE_DIR AND ENKI_LIBRARY AND ENKI_VIEWER_LIBRARY)

# Aseba itself
set(LIB_INSTALL_DIR lib CACHE FILEPATH "Where to install libraries")
set(LIB_VERSION_MAJOR 1) # Must be bumped for incompatible ABI changes
set(LIB_VERSION_MINOR 3)
set(LIB_VERSION_PATCH 2)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})

add_subdirectory(common)

add_subdirectory(transport)

set(ASEBA_CORE_LIBRARIES asebadashelplugins asebacommon ${EXTRA_LIBS})

add_subdirectory(switches)

add_subdirectory(vm)

add_subdirectory(compiler)

add_subdirectory(clients)

add_subdirectory(targets)

add_subdirectory(examples)

add_subdirectory(tests)
