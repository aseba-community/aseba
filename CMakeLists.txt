project(aseba)

# Dashel
# TODO: make Dashel.cmake to perform this

find_path(DASHEL_INCLUDE_DIR dashel.h
	/usr/include/dashel
	/usr/local/include/dashel
)

find_library(DASHEL_LIBRARY dashel
	/usr/lib
	/usr/local/lib
)

include_directories(${DASHEL_INCLUDE_DIR})
set(EXTRA_LIBS ${DASHEL_LIBRARY})

if (WIN32)
	set(EXTRA_LIBS ${EXTRA_LIBS} winspool ws2_32)
else (WIN32)
	if (APPLE)
		find_library(COREFOUNDATION_LIBRARY CoreFoundation)
		find_library(IOKIT_LIBRARY IOKit)
		mark_as_advanced(COREFOUNDATION_LIBRARY)
		mark_as_advanced(IOKIT_LIBRARY)
		set(EXTRA_LIBS ${EXTRA_LIBS} ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY} ${EXTRA_LIBS})
	else (APPLE)
		find_package(PkgConfig)
		pkg_check_modules(HAL hal)
		include_directories(${HAL_INCLUDE_DIRS})
		set(EXTRA_LIBS ${EXTRA_LIBS} ${HAL_LIBRARIES} ${EXTRA_LIBS})
	endif (APPLE)
endif (WIN32)

# Enki

find_path(ENKI_INCLUDE_DIR enki/PhysicalEngine.h
	/usr/include/enki
	/usr/local/include/enki
)

find_library(ENKI_LIBRARY enki
	/usr/lib
	/usr/local/lib
)

find_library(ENKI_VIEWER_LIBRARY enkiviewer
	/usr/lib
	/usr/local/lib
)


if (ENKI_INCLUDE_DIR AND ENKI_LIBRARY AND ENKI_VIEWER_LIBRARY)
	set(ENKI_FOUND 1)
endif (ENKI_INCLUDE_DIR AND ENKI_LIBRARY AND ENKI_VIEWER_LIBRARY)

# Aseba itself

add_definitions(-DASEBA_ASSERT)

add_library(aseba
	vm/vm.c
	vm/natives.c
	transport/buffer/vm-buffer.c
	utils/FormatableString.cpp
	utils/utils.cpp
	compiler/compiler.cpp
	compiler/tree-dump.cpp
	compiler/tree-emit.cpp
	compiler/parser.cpp
	compiler/tree-build.cpp
	compiler/lexer.cpp
	compiler/errors.cpp
	compiler/tree-optimize.cpp
	compiler/analysis.cpp
	msg/msg.cpp
	msg/descriptions-manager.cpp
)

add_subdirectory(switch)

add_subdirectory(cmd)

add_subdirectory(dump)

add_subdirectory(studio)

add_subdirectory(targets/enki-marxbot)

add_subdirectory(targets/challenge)

add_subdirectory(targets/playground)

add_subdirectory(eventlogger)

