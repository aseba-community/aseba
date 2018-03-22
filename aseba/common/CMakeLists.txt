configure_file("version.cpp.tpl" "version.cpp")


add_library(asebacommon
	utils/FormatableString.cpp
	utils/utils.cpp
	utils/HexFile.cpp
	utils/BootloaderInterface.cpp
	msg/msg.cpp
	msg/NodesManager.cpp
	msg/TargetDescription.cpp
	${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)
set_target_properties(asebacommon PROPERTIES VERSION ${LIB_VERSION_STRING})
target_link_libraries(asebacommon PUBLIC aseba_conf dashel)

target_link_libraries(aseba_conf)

install(TARGETS asebacommon 
	LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
	ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
)

add_subdirectory(about)

if(HAS_ZEROCONF_SUPPORT)
	add_subdirectory(zeroconf)
endif()
