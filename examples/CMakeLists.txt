# Examples

# Simulate Aseba is installed system-wide
find_path(aseba_INCLUDE_DIR common/types.h PATHS .. NO_DEFAULT_PATH)
find_path(aseba_INCLUDE_DIR common/types.h PATH_SUFFIXES aseba CMAKE_FIND_ROOT_PATH_BOTH)
include_directories(${aseba_INCLUDE_DIR})
# Otherwise
#find_package(aseba REQUIRED)

# How to find Dashel
#find_package(dashel REQUIRED)
#include_directories(${dashel_INCLUDE_DIRS})

# How to link Dashel and Aseba together
#set(ASEBA_CORE_LIBRARIES asebadashelplugins asebacommon ${dashel_LIBRARIES})

# Different types of examples
add_subdirectory(clients)
add_subdirectory(switches)