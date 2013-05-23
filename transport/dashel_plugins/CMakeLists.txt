# Select plugins following system
if (LINUX)
	set (ASEBADASHEL_PLUGINS_SRC socketcan.cpp)
endif (LINUX)
if (ANDROID) # android overrides Linux
	set (ASEBADASHEL_PLUGINS_SRC android.cpp)
endif (ANDROID)
if ((NOT LINUX) AND (NOT ANDROID))
	set (ASEBADASHEL_PLUGINS_SRC none.cpp)
endif ((NOT LINUX) AND (NOT ANDROID))

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