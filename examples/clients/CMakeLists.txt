
# Aseba
find_path(ASEBA_INCLUDE_DIR common/types.h PATHS ../.. NO_DEFAULT_PATH)
find_path(ASEBA_INCLUDE_DIR common/types.h PATH_SUFFIXES aseba CMAKE_FIND_ROOT_PATH_BOTH)
include_directories(${ASEBA_INCLUDE_DIR})

# Dashel

find_path(DASHEL_INCLUDE_DIR dashel/dashel.h CMAKE_FIND_ROOT_PATH_BOTH)

find_library(DASHEL_LIBRARY dashel CMAKE_FIND_ROOT_PATH_BOTH)

include_directories(${DASHEL_INCLUDE_DIR})
set(EXTRA_LIBS ${DASHEL_LIBRARY})

if (WIN32)
	set(EXTRA_LIBS ${EXTRA_LIBS} winspool ws2_32 setupapi)
endif (WIN32)

set(ASEBA_CORE_LIBRARIES asebacommon ${EXTRA_LIBS})

add_subdirectory(cpp-shell)
add_subdirectory(cpp-qt-gui)


