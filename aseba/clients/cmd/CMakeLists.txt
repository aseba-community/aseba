add_executable(asebacmd
	cmd.cpp
)
target_link_libraries(asebacmd asebacommon)
install(TARGETS asebacmd RUNTIME
	DESTINATION bin
)
