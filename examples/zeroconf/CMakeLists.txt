find_package(DNSSD)

add_executable(asebatargetadvertise
	targetadvertise.cpp
)
target_link_libraries(asebatargetadvertise ${ASEBA_CORE_LIBRARIES} ${DNSSD_LIBRARIES})

add_executable(asebatargetlist
	targetlist.cpp
)
target_link_libraries(asebatargetlist ${ASEBA_CORE_LIBRARIES} ${DNSSD_LIBRARIES})

install(TARGETS asebatargetadvertise asebatargetlist RUNTIME
	DESTINATION bin
)


# Using DashelhubZeroconf

add_executable(asebatargetlist-dashelhub
	targetlist-dashelhub.cpp
)
target_link_libraries(asebatargetlist-dashelhub ${ASEBA_CORE_LIBRARIES} ${DNSSD_LIBRARIES})


# Using ThreadZeroconf

add_executable(asebatargetlist-thread
	targetlist-thread.cpp
)
target_link_libraries(asebatargetlist-thread ${ASEBA_CORE_LIBRARIES} ${DNSSD_LIBRARIES})
