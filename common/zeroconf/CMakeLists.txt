if (DNSSD_FOUND)
	# Zeroconf without Qt
	set(ASEBA_ZEROCONF_SRC
		zeroconf.cpp
		txtrecord.cpp
		target.cpp
		zeroconf-thread.cpp
		zeroconf-dashelhub.cpp
	)
	
	find_library(AVAHI_COMMON_LIBRARIES avahi-common)
	find_library(AVAHI_CLIENT_LIBRARIES avahi-client)
	if (AVAHI_COMMON_LIBRARIES)
		set(ASEBA_ZEROCONF_SRC ${ASEBA_ZEROCONF_SRC} avahi-compat-libdns_sd/compat.c)
	endif ()
	
	add_library(asebazeroconf ${ASEBA_ZEROCONF_SRC})
	set_target_properties(asebazeroconf PROPERTIES VERSION ${LIB_VERSION_STRING})
	target_include_directories(asebazeroconf PUBLIC ${DNSSD_INCLUDE_DIRS})
	
	install(TARGETS asebazeroconf 
		LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
		ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
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
		qt5_wrap_cpp(asebacommon_zeroconf_MOCS
			zeroconf-qt.h
		)
		set(ASEBA_ZEROCONFQT_SRC
			${asebacommon_zeroconf_MOCS}
			zeroconf-qt.cpp
		)
		add_library(asebazeroconfqt ${ASEBA_ZEROCONFQT_SRC})
		set_target_properties(asebazeroconfqt PROPERTIES VERSION ${LIB_VERSION_STRING})
		target_include_directories(asebazeroconfqt PUBLIC ${DNSSD_INCLUDE_DIRS})
		qt5_use_modules(asebazeroconfqt Network)
		
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
endif ()


