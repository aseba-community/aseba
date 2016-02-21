
# Aseba
find_path(ASEBA_INCLUDE_DIR common/types.h PATHS ../.. NO_DEFAULT_PATH)
find_path(ASEBA_INCLUDE_DIR common/types.h PATH_SUFFIXES aseba CMAKE_FIND_ROOT_PATH_BOTH)
include_directories(${ASEBA_INCLUDE_DIR})

# Dashel
find_package(dashel REQUIRED)
include_directories(${dashel_INCLUDE_DIRS})
set(ASEBA_CORE_LIBRARIES asebacommon ${dashel_LIBRARIES})

add_subdirectory(cpp-shell)
add_subdirectory(cpp-qt-gui)
add_subdirectory(scratch)
