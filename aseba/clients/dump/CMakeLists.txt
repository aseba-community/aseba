add_executable(asebadump
	dump.cpp
)
target_link_libraries(asebadump asebadashelplugins asebacommon)
install(TARGETS asebadump RUNTIME
	DESTINATION bin
)
