# emacs: -*- CMake -*-
# kate: replace-tabs off; tab-width 4; indent-width 4; tab-indents true; indent-mode normal
# vim: ts=4:sw=4:noexpandtab

# project name
project(aseba)

# ensure a minimal version of CMake
cmake_minimum_required(VERSION 3.5)
include(FeatureSummary)

# additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)
include(CMakeModules/ParseVersion.cmake)
include(CMakeModules/cpp_features.cmake)
include(CMakeModules/workarounds.cmake)
include(CMakeModules/aseba_conf.cmake)
include(CMakeModules/dashel.cmake)
include(CMakeModules/enki.cmake)
include(CMakeModules/deployqt.cmake)
include(CMakeModules/codesign.cmake)


# testing and defines
enable_testing()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(Threads REQUIRED)

# Qt and Qwt
find_package(Qt5 OPTIONAL_COMPONENTS Core Gui Widgets OpenGL Network Help Concurrent Xml Svg Test)

find_package(Qwt)

if (UNIX AND NOT APPLE)
	find_package(Qt5X11Extras)
	find_package(Qt5DBus)
endif ()

# Zeroconf
include(CMakeModules/zeroconf.cmake)


# Aseba itself
set(LIB_INSTALL_DIR lib CACHE FILEPATH "Where to install libraries")
set(LIB_VERSION_MAJOR 3) # Must be bumped for incompatible ABI changes
set(LIB_VERSION_MINOR 0)
set(LIB_VERSION_PATCH 0)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})


add_subdirectory(aseba)

# Examples for developers
add_subdirectory(examples)

# Unit tests
add_subdirectory(tests)

# User entries
if (UNIX)
	add_subdirectory(menu)
endif()


if(EXISTS ${PROJECT_SOURCE_DIR}/thymio-blockly-standalone/CMakeLists.txt)
	add_subdirectory(${PROJECT_SOURCE_DIR}/thymio-blockly-standalone)
	message("-- Found thymio-blockly-standalone, will bundle blockly")
endif()


feature_summary( WHAT PACKAGES_FOUND PACKAGES_NOT_FOUND ENABLED_FEATURES DISABLED_FEATURES)


if(WIN32)
	add_subdirectory(third_party/bonjour)
	set(ASEBA_SOURCE_DIRECTORY "${PROJECT_SOURCE_DIR}")
	set(CPACK_MODULE_PATH ${PROJECT_SOURCE_DIR}/windows ${CPACK_MODULE_PATH})
	set(CPACK_MONOLITHIC_INSTALL 1)
	configure_file(
		${PROJECT_SOURCE_DIR}/windows/NSIS.definitions.nsh.in
		${CMAKE_CURRENT_BINARY_DIR}/resources/NSIS.definitions.nsh
	)

endif()
include(CPack)







