add_executable(asebaswitch
	switch.cpp
)

target_link_libraries(asebaswitch asebazeroconf)

install(TARGETS asebaswitch RUNTIME
	DESTINATION bin
)
