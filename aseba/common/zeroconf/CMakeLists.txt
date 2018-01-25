add_library(asebazeroconf
	zeroconf.cpp
	txtrecord.cpp
	target.cpp
	zeroconf-thread.cpp
	zeroconf-dashelhub.cpp
)
set_target_properties(asebazeroconf PROPERTIES VERSION ${LIB_VERSION_STRING})
target_link_libraries(asebazeroconf PUBLIC asebacommon zeroconf dashel)


install(TARGETS asebazeroconf
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
	ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
	INCLUDES DESTINATION include/aseba/common/zeroconf
)

set (ASEBAZEROCONF_HDR
	zeroconf.h
	zeroconf-thread.h
	zeroconf-dashelhub.h
)
install(FILES ${ASEBAZEROCONF_HDR}
	DESTINATION include/aseba/common/zeroconf
)

# Zeroconf with Qt
if (Qt5Core_FOUND)
	set(CMAKE_AUTOMOC ON)
	add_library(asebazeroconfqt zeroconf-qt.cpp)
	set_target_properties(asebazeroconfqt PROPERTIES VERSION ${LIB_VERSION_STRING})
	target_include_directories(asebazeroconfqt PUBLIC ${DNSSD_INCLUDE_DIRS})
	target_link_libraries(asebazeroconfqt PUBLIC asebazeroconf Qt5::Core Qt5::Network)


	install(TARGETS asebazeroconfqt
		LIBRARY DESTINATION ${LIB_INSTALL_DIR}
		ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
	)

	set (ASEBAZEROCONFQT_HDR
		zeroconf-qt.h
	)
	install(FILES ${ASEBAZEROCONFQT_HDR}
		DESTINATION include/aseba/common/zeroconf
	)
endif ()


