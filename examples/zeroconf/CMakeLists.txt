find_package(DNSSD)

add_executable(asebatargetadvertise
	targetadvertise.cpp
)
target_link_libraries(asebatargetadvertise ${ASEBA_CORE_LIBRARIES} ${DNSSD_LIBRARIES})
install(TARGETS asebatargetadvertise RUNTIME
	DESTINATION bin
)

add_executable(asebatargetlist
	targetlist.cpp
)
target_link_libraries(asebatargetlist ${ASEBA_CORE_LIBRARIES} ${DNSSD_LIBRARIES})
install(TARGETS asebatargetlist RUNTIME
	DESTINATION bin
)
