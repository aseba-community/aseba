if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
	set(ASEBA_CORE_LIBRARIES
		${ASEBA_CORE_LIBRARIES}
		-ldns_sd
	)
endif(CMAKE_SYSTEM_NAME MATCHES ".*Linux")

add_executable(asebatargetadvertise
	targetadvertise.cpp
)
target_link_libraries(asebatargetadvertise ${ASEBA_CORE_LIBRARIES})
install(TARGETS asebatargetadvertise RUNTIME
	DESTINATION bin
)

add_executable(asebatargetlist
	targetlist.cpp
)
target_link_libraries(asebatargetlist ${ASEBA_CORE_LIBRARIES})
install(TARGETS asebatargetlist RUNTIME
	DESTINATION bin
)
