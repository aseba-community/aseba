# -*- mode: cmake; -*-
# - Try to find libjson include dirs and libraries
# Usage of this module as follows:
# This file defines:
# * LIBJSON_FOUND if protoc was found
# * LIBJSON_LIBRARY The lib to link to (currently only a static unix lib, not
# portable)
# * LIBJSON_INCLUDE The include directories for libjson.

message(STATUS "FindLibJson check")
IF (NOT WIN32)
  include(FindPkgConfig)
  if ( PKG_CONFIG_FOUND )

     pkg_check_modules (LIBPC_JSON json>=0.9)

     set(LIBJSON_DEFINITIONS ${PC_LIBJSON_CFLAGS_OTHER})
  endif(PKG_CONFIG_FOUND)
endif (NOT WIN32)

#
# set defaults
SET(_json_HOME "/usr/local")
SET(_json_INCLUDE_SEARCH_DIRS
  ${CMAKE_INCLUDE_PATH}
  /usr/local/include
  /usr/include
  )

SET(_json_LIBRARIES_SEARCH_DIRS
  ${CMAKE_LIBRARY_PATH}
  /usr/local/lib
  /usr/lib
  )

##
if( "${LIBJSON_HOME}" STREQUAL "")
  if("" MATCHES "$ENV{LIBJSON_HOME}")
    message(STATUS "LIBJSON_HOME env is not set, setting it to /usr/local")
    set (LIBJSON_HOME ${_json_HOME})
  else("" MATCHES "$ENV{LIBJSON_HOME}")
    set (LIBJSON_HOME "$ENV{LIBJSON_HOME}")
  endif("" MATCHES "$ENV{LIBJSON_HOME}")
else( "${LIBJSON_HOME}" STREQUAL "")
  message(STATUS "LIBJSON_HOME is not empty: \"${LIBJSON_HOME}\"")
endif( "${LIBJSON_HOME}" STREQUAL "")
##

message(STATUS "Looking for json in ${LIBJSON_HOME}")

IF( NOT ${LIBJSON_HOME} STREQUAL "" )
    SET(_json_INCLUDE_SEARCH_DIRS ${LIBJSON_HOME}/include ${_json_INCLUDE_SEARCH_DIRS})
    SET(_json_LIBRARIES_SEARCH_DIRS ${LIBJSON_HOME}/lib ${_json_LIBRARIES_SEARCH_DIRS})
    SET(_json_HOME ${LIBJSON_HOME})
ENDIF( NOT ${LIBJSON_HOME} STREQUAL "" )

IF( NOT $ENV{LIBJSON_INCLUDEDIR} STREQUAL "" )
  SET(_json_INCLUDE_SEARCH_DIRS $ENV{LIBJSON_INCLUDEDIR} ${_json_INCLUDE_SEARCH_DIRS})
ENDIF( NOT $ENV{LIBJSON_INCLUDEDIR} STREQUAL "" )

IF( NOT $ENV{LIBJSON_LIBRARYDIR} STREQUAL "" )
  SET(_json_LIBRARIES_SEARCH_DIRS $ENV{LIBJSON_LIBRARYDIR} ${_json_LIBRARIES_SEARCH_DIRS})
ENDIF( NOT $ENV{LIBJSON_LIBRARYDIR} STREQUAL "" )

IF( LIBJSON_HOME )
  SET(_json_INCLUDE_SEARCH_DIRS ${LIBJSON_HOME}/include ${_json_INCLUDE_SEARCH_DIRS})
  SET(_json_LIBRARIES_SEARCH_DIRS ${LIBJSON_HOME}/lib ${_json_LIBRARIES_SEARCH_DIRS})
  SET(_json_HOME ${LIBJSON_HOME})
ENDIF( LIBJSON_HOME )

message("Json search: '${_json_INCLUDE_SEARCH_DIRS}' ${CMAKE_INCLUDE_PATH}")
# find the include files
FIND_PATH(LIBJSON_INCLUDE_DIR libjson/libjson.h
   HINTS
     ${_json_INCLUDE_SEARCH_DIRS}
     ${PC_LIBJSON_INCLUDEDIR}
     ${PC_LIBJSON_INCLUDE_DIRS}
    ${CMAKE_INCLUDE_PATH}
)

# locate the library
IF(WIN32)
  SET(LIBJSON_LIBRARY_NAMES ${LIBJSON_LIBRARY_NAMES} libjson.lib)
ELSE(WIN32)
  SET(LIBJSON_LIBRARY_NAMES ${LIBJSON_LIBRARY_NAMES} libjson.a)
ENDIF(WIN32)
FIND_LIBRARY(LIBJSON_LIBRARY NAMES ${LIBJSON_LIBRARY_NAMES}
  HINTS
    ${_json_LIBRARIES_SEARCH_DIRS}
    ${PC_LIBJSON_LIBDIR}
    ${PC_LIBJSON_LIBRARY_DIRS}
)

# if the include and the program are found then we have it
IF(LIBJSON_INCLUDE_DIR AND LIBJSON_LIBRARY)
  SET(LIBJSON_FOUND "YES")
ENDIF(LIBJSON_INCLUDE_DIR AND LIBJSON_LIBRARY)

# if( NOT WIN32)
#   list(APPEND LIBJSON_LIBRARY "-lrt")
# endif( NOT WIN32)

MARK_AS_ADVANCED(
  LIBJSON_FOUND
  LIBJSON_LIBRARY
  LIBJSON_INCLUDE_DIR
)
