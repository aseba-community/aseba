add_executable(asebaexec
	exec.cpp
)
target_link_libraries(asebaexec ${ASEBA_CORE_LIBRARIES})
install(TARGETS asebaexec RUNTIME
	DESTINATION bin
)
