add_executable(asebacmd
	cmd.cpp
)

target_link_libraries(asebacmd asebacore ${EXTRA_LIBS})

install(TARGETS asebacmd RUNTIME
	DESTINATION bin
	LIBRARY DESTINATION bin
)
