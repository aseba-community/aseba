find_package(DNSSD)
find_package(Threads REQUIRED)
if (DNSSD_FOUND)
	set(HAS_ZEROCONF_SUPPORT ON)
	add_library(zeroconf INTERFACE)
	target_include_directories(zeroconf INTERFACE ${DNSSD_INCLUDE_DIRS})
	target_compile_definitions(zeroconf INTERFACE -DZEROCONF_SUPPORT)
	target_link_libraries(zeroconf INTERFACE ${DNSSD_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
	if (AVAHI_COMMON_LIBRARIES)
		target_compile_definitions(zeroconf INTERFACE -DDNSSD_AVAHI)
		target_link_libraries(zeroconf INTERFACE ${AVAHI_CLIENT_LIBRARIES} ${AVAHI_COMMON_LIBRARIES})
	endif()
	if (WIN32)
		target_link_libraries(zeroconf -lws2_32)
	endif(WIN32)
endif()
