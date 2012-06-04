add_executable(asebaexec
	exec.cpp
)

target_link_libraries(asebaexec asebacore ${EXTRA_LIBS})

install(TARGETS asebaexec RUNTIME
	DESTINATION bin
	LIBRARY DESTINATION lib
)
