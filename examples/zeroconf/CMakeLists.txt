add_executable(asebatargetadvertise
	targetadvertise.cpp
)
target_link_libraries(asebatargetadvertise asebazeroconf ${ASEBA_CORE_LIBRARIES})

# Using DashelhubZeroconf

add_executable(asebatargetlist-dashelhub
	targetlist-dashelhub.cpp
)
target_link_libraries(asebatargetlist-dashelhub asebazeroconf ${ASEBA_CORE_LIBRARIES})

# Using ThreadZeroconf

add_executable(asebatargetlist-thread
	targetlist-thread.cpp
)
target_link_libraries(asebatargetlist-thread asebazeroconf ${ASEBA_CORE_LIBRARIES})

# Using QtZeroconf

if (Qt5Core_FOUND AND Qt5Network_FOUND)
	set(CMAKE_AUTOMOC ON)
	add_executable(asebatargetlist-qt targetlist-qt.cpp)
	target_link_libraries(asebatargetlist-qt asebazeroconfqt ${ASEBA_CORE_LIBRARIES} Qt5::Core Qt5::Network)
endif ()
