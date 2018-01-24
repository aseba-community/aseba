# Select plugins following system
if (ANDROID)
	set (ASEBADASHEL_PLUGINS_SRC android.cpp)
	message(STATUS "Enabling Dashel android plugin")
else (ANDROID)
	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set (ASEBADASHEL_PLUGINS_SRC socketcan.cpp)
		message(STATUS "Enabling Dashel socketcan plugin")
	else (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set (ASEBADASHEL_PLUGINS_SRC none.cpp)
		message(STATUS "System is neither Linux nor Android, no Dashel plugins")
	endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif (ANDROID)

add_library(asebadashelplugins ${ASEBADASHEL_PLUGINS_SRC})
set_target_properties(asebadashelplugins PROPERTIES VERSION ${LIB_VERSION_STRING} 
                                        SOVERSION ${LIB_VERSION_MAJOR})
install(TARGETS asebadashelplugins 
		LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
		ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
)
install(FILES dashel-plugins.h
	DESTINATION include/aseba/transport/dashel_plugins
)