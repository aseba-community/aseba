add_executable(asebaexec
	exec.cpp
)
target_link_libraries(asebaexec asebacommon)
install(TARGETS asebaexec RUNTIME
	DESTINATION bin
)
