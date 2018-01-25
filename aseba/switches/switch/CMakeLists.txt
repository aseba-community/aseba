add_executable(asebaswitch
	switch.cpp
)

target_link_libraries(asebaswitch asebazeroconf asebacommon)

install(TARGETS asebaswitch RUNTIME
	DESTINATION bin
)
