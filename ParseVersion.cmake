# This module checks for SVN / Git binaries, extracts the revision number, and writes it in version.h
# The user can provide a version number through the USER_BUILD_VERSION variable

# version management
if (USER_BUILD_VERSION)
	message(STATUS "User version given: " ${USER_BUILD_VERSION})
	file(WRITE ${CMAKE_BINARY_DIR}/version.h "#define ASEBA_BUILD_VERSION \"${USER_BUILD_VERSION}\"\n")
	return()
endif (USER_BUILD_VERSION)

##########
# detect subversion
##########
find_package(Subversion)

if (Subversion_FOUND)
	message(STATUS "Subversion executable found")
else (Subversion_FOUND)
	message(STATUS "Subversion executable NOT found")
endif (Subversion_FOUND)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn/")
	set(HAS_SVN_REP 1)
endif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn/")

# react accordingly
if (Subversion_FOUND AND HAS_SVN_REP)
	message(STATUS "SVN repository found")
	set(HAS_DYN_VERSION)
	# extract working copy information for SOURCE_DIR into MY_XXX variables
	Subversion_WC_INFO(${CMAKE_CURRENT_SOURCE_DIR} ASEBA)
	# write a file with the SVNVERSION define
	file(WRITE ${CMAKE_BINARY_DIR}/version.h.txt "#define ASEBA_BUILD_VERSION \"svn-${ASEBA_WC_REVISION}\"\n")
	# copy the file to the final header only if the version changes
	# reduces needless rebuilds
	execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different version.h.txt version.h)
	add_custom_target(versionheader ALL DEPENDS ${CMAKE_BINARY_DIR}/version.h)
	set_source_files_properties(${CMAKE_BINARY_DIR}/version.h PROPERTIES GENERATED TRUE HEADER_FILE_ONLY TRUE)
	return()
endif (Subversion_FOUND AND HAS_SVN_REP)

##########
# detect git
##########
find_package(Git QUIET)
if (NOT GIT_FOUND)
  find_program(GIT_EXECUTABLE git NO_CMAKE_FIND_ROOT_PATH)
  if (GIT_EXECUTABLE)
    set(GIT_FOUND TRUE)
  endif (GIT_EXECUTABLE)
endif (NOT GIT_FOUND)

if (GIT_FOUND)
	message(STATUS "Git executable found")
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short --verify HEAD
		OUTPUT_VARIABLE GIT_REV
		ERROR_VARIABLE git_rev_error
		RESULT_VARIABLE git_rev_result
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(${git_rev_result} EQUAL 0)
		set(HAS_GIT_REP 1)
	endif(${git_rev_result} EQUAL 0)
else (GIT_FOUND)
	message(STATUS "Git executable NOT found")
endif (GIT_FOUND)

# react accordingly
if (GIT_FOUND AND HAS_GIT_REP)
	message(STATUS "Git repository found")
	set(HAS_DYN_VERSION)
	# write a file with the GIT_REV define
	file(WRITE ${CMAKE_BINARY_DIR}/version.h.txt "#define ASEBA_BUILD_VERSION \"git-${GIT_REV}\"\n")
	# write NSIS version file (for Windows build)
	file(READ "${PROJECT_SOURCE_DIR}/common/consts.h" COMMON_CONSTS_H)
	string(REGEX MATCH [0-9]+\\.[0-9]+\\.[0-9]+ ASEBA_VERSION ${COMMON_CONSTS_H})
	file(WRITE ${CMAKE_BINARY_DIR}/version.nsi "!define VERSION \"${ASEBA_VERSION}-git-${GIT_REV}\"\n")
	# copy the file to the final header only if the version changes
	# reduces needless rebuilds
	execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different version.h.txt version.h)
	add_custom_target(versionheader ALL DEPENDS ${CMAKE_BINARY_DIR}/version.h)
	set_source_files_properties(${CMAKE_BINARY_DIR}/version.h PROPERTIES GENERATED TRUE HEADER_FILE_ONLY TRUE)
	return()
endif (GIT_FOUND AND HAS_GIT_REP)

##########
# default
##########
message(STATUS "(svn / git executable not found OR not a svn / git repository) and no user version given, version is unknown.")
file(WRITE ${CMAKE_BINARY_DIR}/version.h "#define ASEBA_BUILD_VERSION \"unknown\"\n")

