# Select plugins following system
if (ANDROID)
	set (ASEBADASHEL_PLUGINS_SRC android.cpp)
	message("-- Enabling Dashel android plugin")
else (ANDROID)
	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set (ASEBADASHEL_PLUGINS_SRC socketcan.cpp)
		message("-- Enabling Dashel socketcan plugin")
	else (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set (ASEBADASHEL_PLUGINS_SRC none.cpp)
		message("-- System is neither Linux nor Android, no Dashel plugins")
	endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif (ANDROID)

add_library(asebadashelplugins ${ASEBADASHEL_PLUGINS_SRC})
install(TARGETS asebadashelplugins ARCHIVE
	DESTINATION lib
)

set(ASEBACORE_HDR_DASHEL_PLUGINS
	dashel-plugins.h
)
install(FILES ${ASEBACORE_HDR_DASHEL_PLUGINS}
	DESTINATION include/aseba/transport/dashel_plugins
)