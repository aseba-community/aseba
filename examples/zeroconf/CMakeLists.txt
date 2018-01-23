add_executable(asebatargetadvertise
	targetadvertise.cpp
)
target_link_libraries(asebatargetadvertise ${ASEBA_ZEROCONF_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

# Using DashelhubZeroconf

add_executable(asebatargetlist-dashelhub
	targetlist-dashelhub.cpp
)
target_link_libraries(asebatargetlist-dashelhub ${ASEBA_ZEROCONF_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

# Using ThreadZeroconf

add_executable(asebatargetlist-thread
	targetlist-thread.cpp
)
target_link_libraries(asebatargetlist-thread ${ASEBA_ZEROCONF_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

# Using QtZeroconf

if (Qt5Core_FOUND AND Qt5Network_FOUND)
	set(CMAKE_AUTOMOC ON)
	add_executable(asebatargetlist-qt targetlist-qt.cpp)
	target_link_libraries(asebatargetlist-qt ${ASEBA_ZEROCONFQT_LIBRARIES} ${ASEBA_CORE_LIBRARIES} Qt5::Core Qt5::Network)
endif ()
