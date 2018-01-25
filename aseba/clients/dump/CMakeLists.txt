add_executable(asebadump
	dump.cpp
)
target_link_libraries(asebadump asebacommon)
install(TARGETS asebadump RUNTIME
	DESTINATION bin
)
