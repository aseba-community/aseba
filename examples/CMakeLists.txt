# Different types of examples
add_subdirectory(clients)
add_subdirectory(switches)
if (HAS_ZEROCONF_SUPPORT)
	add_subdirectory(zeroconf)
endif()
