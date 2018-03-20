cmake_minimum_required(VERSION 3.0)
include(FindPackageHandleStandardArgs)

find_program(SEVENZIP_BIN
	NAMES 7z 7za
	HINTS "$ENV{ProgramFiles}/7-zip" "$ENV{ProgramW6432}/7-zip"
	PATH_SUFFIXES bin
	DOC "7zip executable"
	)
find_package_handle_standard_args(SevenZip DEFAULT_MSG SEVENZIP_BIN)
mark_as_advanced(SEVENZIP_BIN)

if(WIN32)
	if(SEVENZIP_FOUND)
		message("-- Bonjour win32 : Will install")
		add_custom_target("ExtractBonjourWin32" ALL)
		add_custom_command(TARGET ExtractBonjourWin32
		                   POST_BUILD
		                   COMMAND ${SEVENZIP_BIN} x -y ${CMAKE_CURRENT_SOURCE_DIR}/bonjoursdksetup.exe Bonjour.msi Bonjour64.msi
		                   COMMENT "Extracting Bonjour"
		                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        #This variables are meant to be used by nsis - therefore they should use \ rather than /
        string(REPLACE "/" "\\" CMAKE_CURRENT_BINARY_DIR_WINDOWS "${CMAKE_CURRENT_BINARY_DIR}")
        set(BONJOUR64_MSI "${CMAKE_CURRENT_BINARY_DIR_WINDOWS}\\Bonjour.msi" CACHE STRING "Path of Bonjour.msi")
        set(BONJOUR_MSI   "${CMAKE_CURRENT_BINARY_DIR_WINDOWS}\\Bonjour64.msi" CACHE STRING "Path of Bonjour64.msi")
	endif()
endif()
