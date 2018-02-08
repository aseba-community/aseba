add_executable(asebaswitch
	switch.cpp
)

target_link_libraries(asebaswitch aseba_conf asebadashelplugins asebacommon)
if(HAS_ZEROCONF_SUPPORT)
	target_link_libraries(asebaswitch asebazeroconf)
endif()

install(TARGETS asebaswitch RUNTIME
	DESTINATION bin
)
