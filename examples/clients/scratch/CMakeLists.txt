# libxml2

if (CMAKE_VERSION VERSION_LESS "3.1")
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
	endif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
else (CMAKE_VERSION VERSION_LESS "3.1")
	set (CMAKE_CXX_STANDARD 11)
endif (CMAKE_VERSION VERSION_LESS "3.1")

find_package(LibXml2)
if (LIBXML2_FOUND)
	
        include_directories(${LIBXML2_INCLUDE_DIR})

	set(scratch_SRCS
		scratch.cpp
	)
	set(scratch_MOCS
		scratch.h
	)
	
	add_executable(asebascratch ${scratch_SRCS} ${scratch_MOCS})
	
	target_link_libraries(asebascratch asebahttphub asebacompiler asebadashelplugins ${LIBXML2_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

	install(TARGETS asebascratch RUNTIME
		DESTINATION bin
	)

endif (LIBXML2_FOUND)

add_subdirectory(e2e-scratch)
