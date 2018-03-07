add_executable(asebaexec
	exec.cpp
)
target_link_libraries(asebaexec asebacommon asebadashelplugins)
install(TARGETS asebaexec RUNTIME
	DESTINATION bin
)
codesign(asebaexec)
