add_executable(asebatargetadvertise
	targetadvertise.cpp
)
target_link_libraries(asebatargetadvertise asebazeroconf asebacommon)

# Using DashelhubZeroconf

add_executable(asebatargetlist-dashelhub
	targetlist-dashelhub.cpp
)
target_link_libraries(asebatargetlist-dashelhub asebazeroconf asebacommon)

# Using ThreadZeroconf

add_executable(asebatargetlist-thread
	targetlist-thread.cpp
)
target_link_libraries(asebatargetlist-thread asebazeroconf asebacommon)

# Using QtZeroconf

if (Qt5Core_FOUND AND Qt5Network_FOUND)
	set(CMAKE_AUTOMOC ON)
	add_executable(asebatargetlist-qt targetlist-qt.cpp)
	target_link_libraries(asebatargetlist-qt asebazeroconfqt asebacommon Qt5::Core Qt5::Network)
endif ()
