# libxml2
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

	configure_file(../../../switches/http/dummynode-1-tick.aesl ${CMAKE_CURRENT_BINARY_DIR}/dummynode-1-tick.aesl COPYONLY)
	configure_file(../../../switches/http/dummynode-1.aesl ${CMAKE_CURRENT_BINARY_DIR}/dummynode-1.aesl COPYONLY)

	install(TARGETS asebascratch RUNTIME
		DESTINATION bin
	)
endif (LIBXML2_FOUND)
