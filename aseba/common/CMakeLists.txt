add_library(asebacommon
	utils/FormatableString.cpp
	utils/utils.cpp
	utils/HexFile.cpp
	utils/BootloaderInterface.cpp
	msg/msg.cpp
	msg/NodesManager.cpp
	msg/TargetDescription.cpp
)
set_target_properties(asebacommon PROPERTIES VERSION ${LIB_VERSION_STRING})
target_link_libraries(asebacommon PUBLIC aseba_conf asebadashelplugins)

target_link_libraries(aseba_conf)

install(TARGETS asebacommon 
	LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
	ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
)

set (ASEBACORE_HDR_UTILS 
	utils/utils.h
	utils/FormatableString.h
)
set (ASEBACORE_HDR_MSG
	msg/msg.h
	msg/NodesManager.h
	msg/TargetDescription.h
)
set (ASEBACORE_HDR_COMMON
	consts.h
	types.h
	productids.h
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

add_subdirectory(about)
add_subdirectory(zeroconf)
