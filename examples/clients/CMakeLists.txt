
# Aseba
find_path(ASEBA_INCLUDE_DIR common/types.h PATHS ../.. NO_DEFAULT_PATH)
find_path(ASEBA_INCLUDE_DIR common/types.h PATH_SUFFIXES aseba CMAKE_FIND_ROOT_PATH_BOTH)
include_directories(${ASEBA_INCLUDE_DIR})

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


set(ASEBA_CORE_LIBRARIES asebacommon ${EXTRA_LIBS})

add_subdirectory(cpp-shell)
add_subdirectory(cpp-qt-gui)


