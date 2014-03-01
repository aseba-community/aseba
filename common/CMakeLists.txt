set (ASEBACOMMON_SRC
	utils/FormatableString.cpp
	utils/utils.cpp
	utils/HexFile.cpp
	utils/BootloaderInterface.cpp
	msg/msg.cpp
	msg/descriptions-manager.cpp
)
add_library(asebacommon ${ASEBACOMMON_SRC})
set_target_properties(asebacommon PROPERTIES VERSION ${LIB_VERSION_STRING} 
                                        SOVERSION ${LIB_VERSION_MAJOR})

install(TARGETS asebacommon 
		LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
		ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
) 

set (ASEBA_HDR_UTILS 
	utils/utils.h
	utils/FormatableString.h
)
set (ASEBA_HDR_MSG
	msg/msg.h
	msg/descriptions-manager.h
)
set (ASEBA_HDR_COMMON
	consts.h
	types.h
)
install(FILES ${ASEBACORE_HDR_UTILS}
	DESTINATION include/aseba/common/utils
)
install(FILES ${ASEBACORE_HDR_MSG}
	DESTINATION include/aseba/common/msg
)
install(FILES ${ASEBACORE_HDR_COMMON}
	DESTINATION include/aseba/common
)
